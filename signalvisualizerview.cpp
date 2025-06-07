#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsLineItem>
#include <QCheckBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
#include "signalvisualizerview.h"
#include "signalvisualizerwidget.h"
#include "proxyitem.h"

SignalVisualizerView::SignalVisualizerView(SignalVisualizerWidget* signalVisualizerWidget, QWidget *parent)
    : QGraphicsView(parent),
    m_signalVisualizerWidget(signalVisualizerWidget),
    m_scaleFactor(1.15),
    m_hoveredItem(nullptr) {
    
    connect(m_signalVisualizerWidget->getModel(), &SignalVisualizer::colorizeFinished, this, &SignalVisualizerView::updateLegend);
    connect(m_signalVisualizerWidget->getModel(), &SignalVisualizer::colorizeFinished,
    this, [this]() {
        m_signalVisualizerWidget->getModel() -> updateNetColors(m_showTypes);
    });
    connect(m_signalVisualizerWidget->getModel(), &SignalVisualizer::legendUpdated, this, &SignalVisualizerView::updateLegend);
    connect(m_signalVisualizerWidget->getModel(), &SignalVisualizer::comboUpdated, this, &SignalVisualizerView::updateDesignationCombo);
    connect(m_signalVisualizerWidget->getModel(), &SignalVisualizer::comboUpdated, this, &SignalVisualizerView::updateTypeCombo);

    m_circuitInstance = signalVisualizerWidget -> getCircuit();
    createEditor();
    applyStyles();
    
    fillItemsForColorComboBox(m_designationColorCombo);
    fillItemsForColorComboBox(m_typeColorCombo);

    displayConnecors(m_circuitInstance);
    displayComponents(m_circuitInstance);
    displayNodes(m_circuitInstance);
    signalVisualizerWidget -> getModel() -> colorizeCircuit();
}

void SignalVisualizerView::createEditor() {
    m_tooltipLabel = new QLabel(this);
    m_tooltipLabel->setVisible(false);

    m_legendOverlay = new QWidget(this);

    m_layout = new QVBoxLayout(m_legendOverlay);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);
    m_legendOverlay->setLayout(m_layout);
    m_legendOverlay->hide();

    m_checkboxOverlay = new QWidget(this);
    m_checkboxLayout = new QVBoxLayout(m_checkboxOverlay);
    m_checkboxLayout->setContentsMargins(5, 5, 5, 5);
    m_checkboxLayout->setSpacing(5);

    m_hideCompPosDesignationCheckbox = new QCheckBox("Скрыть обозначения", m_checkboxOverlay);
    m_checkboxLayout->addWidget(m_hideCompPosDesignationCheckbox);
    connect(m_hideCompPosDesignationCheckbox, &QCheckBox::stateChanged,
            this, &SignalVisualizerView::toggleCompPosDesignationVisibility);

    m_hideCompTextCheckbox = new QCheckBox("Скрыть подписи", m_checkboxOverlay);
    m_checkboxLayout->addWidget(m_hideCompTextCheckbox);
    connect(m_hideCompTextCheckbox, &QCheckBox::stateChanged,
            this, &SignalVisualizerView::toggleCompTextVisibility);

    m_hideCompLabelCheckbox = new QCheckBox("Скрыть значения", m_checkboxOverlay);
    m_checkboxLayout->addWidget(m_hideCompLabelCheckbox);
    connect(m_hideCompLabelCheckbox, &QCheckBox::stateChanged,
            this, &SignalVisualizerView::toggleCompLabelVisibility);

    m_lineEditOverlay = new QWidget(this);
    m_lineEditOverlay->setFixedSize(300, 300);
    m_lineEditOverlay->move(width() - 220, 10);
    m_lineEditLayout = new QFormLayout(m_lineEditOverlay);
    m_lineEditOverlay->setLayout(m_lineEditLayout);

    m_signalDesignationCombo = new QComboBox(m_lineEditOverlay);
    m_lineEditLayout->addRow("Обозначение сигнала:", m_signalDesignationCombo);
    m_designationColorCombo  = new QComboBox(m_lineEditOverlay);
    m_lineEditLayout->addRow("Цвет обозначения сигнала:", m_designationColorCombo);
    m_manageDesignationsButton = new QPushButton("Управление обозначениями...", m_lineEditOverlay);
    m_lineEditLayout->addRow(m_manageDesignationsButton);
    connect(m_manageDesignationsButton, &QPushButton::clicked, this, &SignalVisualizerView::signalDesignationsManager);
    connect(m_signalDesignationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this](int index){
        if(index == -1) return;
        updateColorComboForDesignation(m_signalDesignationCombo->currentText());
        updateThicknessSpinForDesignation(m_signalDesignationCombo->currentText());
    });

    m_designationInfoEdit = new QTextEdit(m_lineEditOverlay);
    m_lineEditLayout->addRow("Информация об обозначении:", m_designationInfoEdit);

    m_signalTypeCombo = new QComboBox(m_lineEditOverlay);
    m_lineEditLayout->addRow("Тип сигнала:", m_signalTypeCombo);
    m_typeColorCombo  = new QComboBox(m_lineEditOverlay);
    m_lineEditLayout->addRow("Цвет типа сигнала:", m_typeColorCombo);

    m_manageTypeButton = new QPushButton("Управление типами...", m_lineEditOverlay);
    m_lineEditLayout->addRow(m_manageTypeButton);
    connect(m_manageTypeButton, &QPushButton::clicked, this, &SignalVisualizerView::signalTypesManager);
    connect(m_signalTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this](int index){
        if(index == -1) return;
        updateColorComboForType(m_signalTypeCombo->currentText());
    });
    m_typeInfoEdit = new QTextEdit(m_lineEditOverlay);
    m_lineEditLayout->addRow("Информация о типе:", m_typeInfoEdit);

    m_thicknessSpin = new QSpinBox(m_lineEditOverlay);
    m_thicknessSpin->setRange(1, 10);
    m_thicknessSpin->setValue(3);
    m_lineEditLayout->addRow("Толщина линии:", m_thicknessSpin);

    m_buttonLayout = new QHBoxLayout();
    m_applyButton = new QPushButton("Применить", m_lineEditOverlay);
    m_buttonLayout->addWidget(m_applyButton);
    connect(m_applyButton, &QPushButton::clicked, this, &SignalVisualizerView::applyChanges);

    m_resetButton = new QPushButton("Сброс", m_lineEditOverlay);
    m_buttonLayout->addWidget(m_resetButton);
    connect(m_resetButton, &QPushButton::clicked, this, &SignalVisualizerView::resetSelection);

    m_lineEditLayout->addRow(m_buttonLayout);
    m_lineEditOverlay->hide();

    setMouseTracking(true);
}

void SignalVisualizerView::applyStyles() {
    m_tooltipLabel->setStyleSheet("background-color: white; border: 1px solid black; padding: 5px;");
    m_legendOverlay->setStyleSheet("background-color: white; border: 1px solid black;");
    m_checkboxOverlay->setStyleSheet("background-color: white; border: 1px solid black;");
    m_checkboxOverlay->setStyleSheet(R"(
        QCheckBox { spacing: 8px; font-size: 12px; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border: 1px solid #666; border-radius: 3px;
            background: #fff;
        }
        QCheckBox::indicator:checked {
            background:rgb(136, 192, 159); border: 1px solidrgb(91, 116, 101);
        }
    )");
    m_lineEditOverlay->setStyleSheet("background-color: rgba(255, 255, 255, 200); border: 1px solid black;");
    m_lineEditOverlay->setStyleSheet(R"(
        /* Сам контейнер overlay */
        QWidget {
            background-color: rgba(255, 255, 255, 230);
            border: 1px solid #444444;
            border-radius: 6px;
        }
        /* Уменьшенные ComboBox-ы */
        QComboBox {
            background-color: rgba(255, 255, 255, 255);
            border: 1px solid #888888;
            border-radius: 4px;
            padding: 2px 4px;       /* меньше отступов */
            min-height: 20px;       /* чуть поменьше по высоте */
            font-size: 12px;        /* мелкий шрифт */
            max-width: 150px;       /* опционально: ограничить ширину */
        }
        QComboBox::drop-down {
            border: none;
            width: 16px;            /* узкий «треугольник» */
        }
        QComboBox QAbstractItemView {
            background-color: rgba(255, 255, 255, 255);
            border: 1px solid #888888;
            border-radius: 4px;
            selection-background-color: #a39f9f;
            font-size: 12px;        /* чтобы выпадашка тоже была маленькая */
            max-height: 120px;
        }
    )");
    m_signalDesignationCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: white;"
        "   color: black;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: white;"
        "   color: black;"
        "   selection-background-color: lightgray;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "   background-color: #e0e0e0;"
        "   color: black;"
        "}"
    );
    m_designationColorCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: white;"
        "   color: black;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: white;"
        "   color: black;"
        "   selection-background-color: lightgray;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "   background-color: #e0e0e0;"
        "   color: black;"
        "}"
    );
    m_signalTypeCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: white;"
        "   color: black;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: white;"
        "   color: black;"
        "   selection-background-color: lightgray;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "   background-color: #e0e0e0;"
        "   color: black;"
        "}"
    );
    m_typeColorCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: white;"
        "   color: black;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: white;"
        "   color: black;"
        "   selection-background-color: lightgray;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "   background-color: #e0e0e0;"
        "   color: black;"
        "}"
    );
    m_applyButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgb(88, 214, 141);  /* нежно‑зелёный */
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: rgb(64, 168, 105);  /* чуть темнее при наведении */
        }
        QPushButton:pressed {
            background-color: rgb(46, 154, 76);   /* ещё темнее при нажатии */
        }
    )");
    m_resetButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgb(202, 175, 206);
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: rgb(158, 138, 161);
        }
        QPushButton:pressed {
            background-color: rgb(120, 100, 120);
        }
    )");
    QString textEditStyle = R"(
        QTextEdit {
            font-size: 12px;
            border: 1px solid #888888;
            border-radius: 4px;
            padding: 2px;
        }
        QScrollBar:vertical {
            width: 12px;
        }
    )";
    // m_designationInfoEdit->setStyleSheet(textEditStyle);
    // m_typeInfoEdit->setStyleSheet(textEditStyle);
}

void SignalVisualizerView::signalDesignationsManager() {
    QDialog dialog(this);
    dialog.setWindowTitle("Управление обозначениями сигналов");
    dialog.setFixedSize(200, 80);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QComboBox *designationsCombo = new QComboBox();
    designationsCombo->addItems(getCurrentDesignations(m_signalDesignationCombo));
    designationsCombo->setCurrentIndex(m_signalDesignationCombo->currentIndex() != -1 ? m_signalDesignationCombo->currentIndex() : 0);
    mainLayout->addWidget(designationsCombo);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnSelect = new QPushButton("Выбрать");
    QPushButton *btnAdd = new QPushButton("Добавить");
    QPushButton *btnRemove = new QPushButton("Удалить");
    btnLayout->addWidget(btnSelect);
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    mainLayout->addLayout(btnLayout);

    QPushButton *btnBack = new QPushButton("Назад");
    btnBack->setFixedWidth(80);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);

    connect(btnBack, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnSelect, &QPushButton::clicked, &dialog, [&]() {
        int selectedIndex = designationsCombo->currentIndex();
        if (selectedIndex != -1) {
            m_signalDesignationCombo->setCurrentIndex(selectedIndex);
        }
        dialog.accept();
    });
    connect(btnAdd, &QPushButton::clicked, &dialog, [&]() {
        bool ok;
        QString newDesignation = QInputDialog::getText(
            &dialog,
            "Новое обозначение сигнала",
            "Введите название:",
            QLineEdit::Normal,
            "",
            &ok
        );

        if (ok && !newDesignation.isEmpty()) {
            if (designationsCombo->findText(newDesignation) == -1) {
                m_signalDesignationCombo->addItem(newDesignation);
                designationsCombo->addItem(newDesignation);
                designationsCombo->setCurrentIndex(designationsCombo->findText(newDesignation));
            }
        }
    });
    connect(btnRemove, &QPushButton::clicked, &dialog, [&]() {
        if (designationsCombo->count() == 0) return;

        QString current = designationsCombo->currentText();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(
            this,
            "Подтверждение удаления",
            QString("Вы уверены, что хотите удалить обозначение сигнала \"%1\"?").arg(current),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            int mainIndex = m_signalDesignationCombo->findText(current);

            if (mainIndex != -1) {
                m_signalDesignationCombo->removeItem(mainIndex);
                designationsCombo->removeItem(designationsCombo->currentIndex());
                m_signalVisualizerWidget-> getModel()->removeDesignationForConnections(current);
                updateLegend();
                updateDesignationCombo();
                m_signalTypeCombo->setCurrentIndex(-1);
                m_typeColorCombo->setCurrentIndex(-1);

                m_signalDesignationCombo->setCurrentIndex(-1);
                m_designationColorCombo->setCurrentIndex(-1);
                m_thicknessSpin->setValue(3);
            }
        }
    });

    dialog.exec();
}

void SignalVisualizerView::signalTypesManager() {
    QDialog dialog(this);
    dialog.setWindowTitle("Управление типами");
    dialog.setFixedSize(200, 80);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItems(getCurrentDesignations(m_signalTypeCombo));
    typeCombo->setCurrentIndex(m_signalTypeCombo->currentIndex() != -1 ? m_signalTypeCombo->currentIndex() : 0);
    mainLayout->addWidget(typeCombo);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnSelect = new QPushButton("Выбрать");
    QPushButton *btnAdd = new QPushButton("Добавить");
    QPushButton *btnRemove = new QPushButton("Удалить");
    btnLayout->addWidget(btnSelect);
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    mainLayout->addLayout(btnLayout);

    QPushButton *btnBack = new QPushButton("Назад");
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);

    connect(btnBack, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnSelect, &QPushButton::clicked, &dialog, [&]() {
        m_signalTypeCombo->setCurrentText(typeCombo->currentText());
        dialog.accept();
    });
    connect(btnAdd, &QPushButton::clicked, &dialog, [&]() {
        bool ok;
        QString newCat = QInputDialog::getText(&dialog, "Новый тип", "Введите название:", QLineEdit::Normal, "", &ok);
        if (ok && !newCat.isEmpty() && typeCombo->findText(newCat) == -1) {
            m_signalTypeCombo->addItem(newCat);
            typeCombo->addItem(newCat);
            typeCombo->setCurrentIndex(typeCombo->findText(newCat));
        }
    });
    connect(btnRemove, &QPushButton::clicked, &dialog, [&]() {
        if (typeCombo->count() == 0) return;

        QString current = typeCombo->currentText();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(
            this,
            "Подтверждение удаления",
            QString("Удалить тип \"%1\" и все связанные с ним обозначения?").arg(current),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            int mainIndex = m_signalTypeCombo->findText(current);

            if (mainIndex != -1) {
                m_signalDesignationCombo->removeItem(mainIndex);
                typeCombo->removeItem(typeCombo->currentIndex());
                m_signalVisualizerWidget-> getModel()->removeTypeForConnections(current);
                updateLegend();
                updateDesignationCombo();
                updateTypeCombo();
                m_signalTypeCombo->setCurrentIndex(-1);
                m_typeColorCombo->setCurrentIndex(-1);
                m_signalDesignationCombo->setCurrentIndex(-1);
                m_designationColorCombo->setCurrentIndex(-1);
            }
        }
    });

    dialog.exec();
}

QStringList SignalVisualizerView::getCurrentDesignations(QComboBox* combo) {
    QStringList designations;
    for(int i = 0; i < combo->count(); ++i)
        designations << combo->itemText(i);
    return designations;
}

void SignalVisualizerView::updateColorComboForDesignation(const QString &designation) {
    if(designation.isEmpty()) return;

    QList<QColor> colors = m_signalVisualizerWidget -> getModel()->getColorsByDesignation(designation);
    if(colors.size() == 1) {
        QColor targetColor = colors.first();
        for(int i = 0; i < m_designationColorCombo->count(); ++i) {
            QColor itemColor = m_designationColorCombo->itemData(i, Qt::UserRole).value<QColor>();
            if(itemColor == targetColor) {
                m_designationColorCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void SignalVisualizerView::updateColorComboForType(const QString &type) {
    if(type.isEmpty()) return;

    QList<QColor> colors = m_signalVisualizerWidget -> getModel()->getColorsByType(type);
    if(colors.size() == 1) {
        QColor targetColor = colors.first();
        for(int i = 0; i < m_typeColorCombo->count(); ++i) {
            QColor itemColor = m_typeColorCombo->itemData(i, Qt::UserRole).value<QColor>();
            if(itemColor == targetColor) {
                m_typeColorCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void SignalVisualizerView::updateThicknessSpinForDesignation(const QString &designation) {
    if(designation.isEmpty()) return;

    QList<int> thicknesses = m_signalVisualizerWidget -> getModel()->getThicknessesByDesignation(designation);
    if (thicknesses.size() == 1) {
        int targetThickness = thicknesses.first();
        m_thicknessSpin->setValue(targetThickness);
    }
}

void SignalVisualizerView::fillItemsForColorComboBox(QComboBox* comboBox) {
    QList<QPair<QColor, QString>> colors = {
        {Qt::black, "Черный"},
        {Qt::red, "Красный"},
        {Qt::blue, "Синий"},
        {Qt::green, "Зеленый"},
        {Qt::yellow, "Желтый"},
        {Qt::magenta, "Пурпурный"}
    };

    for (const auto& color : colors) {
        QPixmap pixmap(16, 16);
        pixmap.fill(color.first);
        QPainter painter(&pixmap);
        painter.setPen(Qt::black);
        painter.drawRect(0, 0, 15, 15);
        comboBox->addItem(QIcon(pixmap), color.second);
        comboBox->setItemData(comboBox->count() - 1, color.first, Qt::UserRole);
    }
}

void SignalVisualizerView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        scale(m_scaleFactor, m_scaleFactor);
    } else {
        scale(1.0 / m_scaleFactor, 1.0 / m_scaleFactor);
    }
}

void SignalVisualizerView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsLineItem* foundLine = nullptr;
        QRectF pickArea(scenePos.x() - 2, scenePos.y() - 2, 4, 4);

        const auto items = scene()->items(pickArea, Qt::IntersectsItemShape, Qt::DescendingOrder);
        for (QGraphicsItem* item : items) {
            if (QGraphicsLineItem* line = dynamic_cast<QGraphicsLineItem*>(item)) {
                foundLine = line;
                break;
            }
        }
        if (foundLine) {
            QList<QGraphicsLineItem*> newLineGroup = m_signalVisualizerWidget->getModel() ->getGroupByLine(foundLine);
            if (m_selectedLineItem && newLineGroup != m_selectedLineGroup) {
                deselectLine(m_selectedLineGroup);
                m_selectedLineGroup.clear();
            }
            m_selectedLineItem = foundLine;
            m_selectedLineGroup = newLineGroup;
            selectLine(m_selectedLineGroup);
        } else {
            if (m_selectedLineItem) {

                deselectLine(m_selectedLineGroup);
                m_selectedLineGroup.clear();
                m_selectedLineItem = nullptr;
            }
            hideEditor();
        }
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        m_lastMousePosition = event->pos();
    }
    QGraphicsView::mousePressEvent(event);
}

void SignalVisualizerView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePosition;
        m_lastMousePosition = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        setCursor(Qt::ClosedHandCursor);
    } else {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsLineItem* foundLine = nullptr;
        QRectF pickArea(scenePos.x() - 2, scenePos.y() - 2, 4, 4);
        const auto items = scene()->items(pickArea, Qt::IntersectsItemShape, Qt::DescendingOrder);

        for (QGraphicsItem* item : items) {
            if (QGraphicsLineItem* line = dynamic_cast<QGraphicsLineItem*>(item)) {
                foundLine = line;
                break;
            }
        }

        if (foundLine && foundLine != m_hoveredItem) {
            m_hoveredItem = foundLine;
            QString info = QString();
            if(isShowingTypes()){
                info = m_signalVisualizerWidget->getModel() ->getDesignationInfoByLine(foundLine);
            } else {
                info = m_signalVisualizerWidget->getModel() ->getTypeInfoByLine(foundLine);
            }
            if (!info.isEmpty()) {
                m_tooltipLabel->setText(info);
                m_tooltipLabel->move(event->pos() + QPoint(15, 15));
                m_tooltipLabel->show();
            }
        } else if (!foundLine) {
            m_tooltipLabel->hide();
            m_hoveredItem = nullptr;
        }
    }
    QGraphicsView::mouseMoveEvent(event);
}

void SignalVisualizerView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void SignalVisualizerView::focusOutEvent(QFocusEvent *event) {
    m_isPanning = false;
    setCursor(Qt::ArrowCursor);
    QGraphicsView::focusOutEvent(event);
}

void SignalVisualizerView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    updateOverlayPosition();
    updateCheckboxOverlayPosition();
    updateEditorOverlayPosition();
}

void SignalVisualizerView::setLegend(const QVector<LegendItem> &items) {
    m_legendItems = items;
    updateLegendOverlay();
}

const QList<QGraphicsLineItem*>& SignalVisualizerView::getSelectedLineGroup() const {
    return m_selectedLineGroup;
}

void SignalVisualizerView::updateLegendOverlay() {
    if (m_legendItems.isEmpty()) {
        m_legendOverlay->hide();
        return;
    }
    delete m_legendOverlay->layout();
    m_layout = new QVBoxLayout(m_legendOverlay);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);

    for (const LegendItem &item : m_legendItems) {
        QLabel *colorLabel = new QLabel();
        colorLabel->setFixedSize(20, 20);
        colorLabel->setStyleSheet(
            QString("background-color: %1; border: 1px solid black;").arg(item.color.name()));

        QLabel *textLabel = new QLabel(item.label);
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(colorLabel);
        itemLayout->addWidget(textLabel);
        m_layout->addLayout(itemLayout);
    }

    m_legendOverlay->setLayout(m_layout);
    m_legendOverlay->show();
    qApp->processEvents();
    m_legendOverlay->adjustSize();
    updateOverlayPosition();
}

void SignalVisualizerView::updateLegend() {
    m_legendItems.clear();
    QSet<QPair<QString, QColor>> items;
    for (const auto &net : m_signalVisualizerWidget -> getModel()-> m_netConnections) {
        if (isShowingTypes()) {
            if (!net.type.isEmpty() && net.typeColor.isValid()) {
                items.insert({net.type, net.typeColor});
            }
        } else {
            if (!net.designation.isEmpty() && net.designationColor.isValid()) {
                items.insert({net.designation, net.designationColor});
            }
        }
    }

    QList<QPair<QColor, QString>> sortedItems;
    for (const auto& item : items) {
        sortedItems.append(qMakePair(item.second, item.first));
    }

    // Сортировка по HSL (Hue -> Saturation -> Lightness)
    std::sort(sortedItems.begin(), sortedItems.end(),
        [](const QPair<QColor, QString>& a, const QPair<QColor, QString>& b) {
            int hA = a.first.hslHue() == -1 ? 0 : a.first.hslHue();
            int hB = b.first.hslHue() == -1 ? 0 : b.first.hslHue();
            if (hA != hB) return hA < hB;
            
            int sA = a.first.hslSaturation();
            int sB = b.first.hslSaturation();
            if (sA != sB) return sA < sB;
            
            return a.first.lightness() < b.first.lightness();
        });

    for (const auto& item : sortedItems) {
        m_legendItems.append({item.first, item.second});
    }
    setLegend(m_legendItems);
}

void SignalVisualizerView::updateOverlayPosition() {
    int margin = 10;
    QSize size = m_legendOverlay->sizeHint();
    m_legendOverlay->setGeometry(
        margin,
        height() - size.height() - margin,
        size.width(),
        size.height()
    );
}

void SignalVisualizerView::updateCheckboxOverlayPosition() {
    int margin = 10;
    QSize size = m_checkboxOverlay->sizeHint();
    int x = width() - size.width() - margin;
    int y = height() - size.height() - margin;
    m_checkboxOverlay->setGeometry(x, y, size.width(), size.height());
}

void SignalVisualizerView::toggleDisplayMode(bool showCategories) {
    m_showTypes = showCategories;
    updateLegend();

    for (auto it = m_signalVisualizerWidget -> getModel()->m_netConnections.begin(); it != m_signalVisualizerWidget -> getModel()->m_netConnections.end(); ++it) {
        const QList<QGraphicsLineItem*>& lines = it.value().lineList;

        QColor colorToApply;
        if (showCategories) {
            colorToApply = it.value().typeColor.isValid() ? it.value().typeColor : Qt::gray;
        } else {
            colorToApply = it.value().designationColor.isValid() ? it.value().designationColor : Qt::gray;
        }

        for (QGraphicsLineItem* line : lines) {
            if (line) {
                QPen pen = line->pen();
                pen.setColor(colorToApply);
                line->setPen(pen);
            }
        }
    }
}

void SignalVisualizerView::toggleCompLabelVisibility(int state) {
    bool checked = (state == Qt::Checked);
    if (checked) {
        setCompLabelVisibility(false);
    } else {
        setCompLabelVisibility(true);
    }
}

void SignalVisualizerView::toggleCompTextVisibility(int state) {
    bool checked = (state == Qt::Checked);
    if (checked) {
        setCompTextVisibility(false);
    } else {
        setCompTextVisibility(true);
    }
}

void SignalVisualizerView::toggleCompPosDesignationVisibility(int state) {
    bool checked = (state == Qt::Checked);
    if (checked) {
        setCompPosDesignationVisibility(false);
    } else {
        setCompPosDesignationVisibility(true);
    }
}

void SignalVisualizerView::updateEditorOverlayPosition() {
    int rightMargin = 10;
    int topMargin = 10;
    int newX = width() - m_lineEditOverlay->width() - rightMargin;
    int newY = topMargin;

    m_lineEditOverlay->move(newX, newY);
}

void SignalVisualizerView::selectLine(QList<QGraphicsLineItem*>& lineGroup) {
    m_selectedLineGroup = lineGroup;
    if (!lineGroup.isEmpty()) {
        m_lineEditOverlay->show();
        m_signalVisualizerWidget -> getModel() -> applyColorToLineGroup(QColorConstants::Svg::orange,lineGroup);

        QString lineDesignation = m_signalVisualizerWidget -> getModel()->getDesignationByGroup(lineGroup);
        int designationIndex = m_signalDesignationCombo->findText(lineDesignation);
        m_signalDesignationCombo->setCurrentIndex(designationIndex);

        QColor designationLineColor = m_signalVisualizerWidget -> getModel()->getDesignationColorByGroup(lineGroup);
        int designationColorIndex = -1;
        for (int i = 0; i < m_designationColorCombo->count(); ++i) {
            QColor itemColor = m_designationColorCombo->itemData(i, Qt::UserRole).value<QColor>();
            if (itemColor == designationLineColor) {
                designationColorIndex = i;
                break;
            }
        }
        m_designationColorCombo->setCurrentIndex(designationColorIndex);
        
        QString designationInfo = m_signalVisualizerWidget->getModel()->getDesignationInfoByGroup(lineGroup);
        m_designationInfoEdit->setPlainText(designationInfo);

        QString lineType = m_signalVisualizerWidget -> getModel()->getTypeByGroup(lineGroup);
        int typeIndex = m_signalTypeCombo->findText(lineType);
        m_signalTypeCombo->setCurrentIndex(typeIndex);

        QColor typeLineColor = m_signalVisualizerWidget -> getModel()->getTypeColorByGroup(lineGroup);
        int typeColorIndex = -1;
        for (int i = 0; i < m_typeColorCombo->count(); ++i) {
            QColor itemColor = m_typeColorCombo->itemData(i, Qt::UserRole).value<QColor>();
            if (itemColor == typeLineColor) {
                typeColorIndex = i;
                break;
            }
        }
        m_typeColorCombo->setCurrentIndex(typeColorIndex);

        QString typeInfo = m_signalVisualizerWidget->getModel()->getTypeInfoByGroup(lineGroup);
        m_typeInfoEdit->setPlainText(typeInfo);

        int thickness = lineGroup.first()->pen().width();
        m_thicknessSpin->setValue(thickness);
    }
}

void SignalVisualizerView::deselectLine(QList<QGraphicsLineItem*>& lineGroup) {
    if (!lineGroup.isEmpty()) {
        QColor originalColor = m_signalVisualizerWidget -> getModel() -> getLineColorByGroup(lineGroup, isShowingTypes());
        m_signalVisualizerWidget -> getModel() -> applyColorToLineGroup(originalColor,lineGroup);
    }
}

void SignalVisualizerView::resetSelection() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::warning(
        this,
        "Подтверждение сброса",
        "Вы уверены, что хотите сбросить выделение?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_signalVisualizerWidget -> getModel() -> resetConnectionsByGroup(m_selectedLineGroup);
        updateLegend();
        m_signalDesignationCombo->setCurrentIndex(-1);
        m_designationColorCombo->setCurrentIndex(-1);
        m_designationInfoEdit->clear();
        m_signalTypeCombo->setCurrentIndex(-1);
        m_typeColorCombo->setCurrentIndex(-1);
        m_typeInfoEdit->clear();
        m_thicknessSpin->setValue(3);
    }
}

void SignalVisualizerView::clearSelection() {
    m_selectedLineItem = nullptr;
    m_selectedLineGroup.clear();
}

void SignalVisualizerView::hideEditor() {
    m_lineEditOverlay->hide();
}

void SignalVisualizerView::applyChanges() {
    if (m_selectedLineGroup.isEmpty()) return;

    QStringList errors;
    if (m_signalDesignationCombo->currentIndex() == -1) errors << "обозначение сигнала";
    if (m_designationColorCombo->currentIndex() == -1) errors << "цвет обозначения";
    if (m_signalTypeCombo->currentIndex() == -1) errors << "тип сигнала";
    if (m_typeColorCombo->currentIndex() == -1) errors << "цвет типа";

    if (!errors.isEmpty()) {
        QMessageBox::warning(
            this,
            "Ошибка",
            "Не заполнены поля: " + errors.join(", ") + "!"
        );
        return;
    }

    QString newDesignation = m_signalDesignationCombo->currentText();
    QColor newDesignationColor = m_designationColorCombo->currentData(Qt::UserRole).value<QColor>();
    QString newDesignationInfo = m_designationInfoEdit->toPlainText();

    QString newType = m_signalTypeCombo->currentText();
    QColor newTypeColor = m_typeColorCombo->currentData(Qt::UserRole).value<QColor>();
    QString newTypeInfo = m_typeInfoEdit->toPlainText();
    int newThickness = m_thicknessSpin->value();

    bool isSystemType = m_signalVisualizerWidget->getModel()->isSystemType(newType);
    bool isSystemDesignationType = m_signalVisualizerWidget->getModel()->isSystemDesignationType(newType);
    QString oldType = m_signalVisualizerWidget->getModel()->getTypeByGroup(m_selectedLineGroup);
    QColor oldTypeColor = m_signalVisualizerWidget->getModel()->getLineColorByGroup(m_selectedLineGroup, true);
    QColor oldDesignationColor = m_signalVisualizerWidget->getModel()->getLineColorByGroup(m_selectedLineGroup, false);
    
    if (isSystemType && (newTypeColor != oldTypeColor)) errors << "цвет типа";
    if (isSystemType && isSystemDesignationType && (newDesignationColor != oldDesignationColor)) errors << "цвет обозначения";
    if (!errors.isEmpty()) {
        QMessageBox::warning(
            this,
            "Ошибка",
            "Системный " + errors.join(", ") + " изменять нельзя!"
        );
        return;
    }

    m_signalVisualizerWidget -> getModel() -> setDesignationByGroup(newDesignation, m_selectedLineGroup);
    m_signalVisualizerWidget -> getModel() -> setTypeByGroup(newType, m_selectedLineGroup);
    m_signalVisualizerWidget -> getModel() -> setDesignationLineColorByGroup(newDesignationColor, m_selectedLineGroup);
    m_signalVisualizerWidget -> getModel() -> setTypeLineColorByGroup(newTypeColor, m_selectedLineGroup);
    m_signalVisualizerWidget -> getModel() -> setDesignationInfoByGroup(newDesignationInfo, m_selectedLineGroup);
    m_signalVisualizerWidget -> getModel() -> setTypeInfoByGroup(newTypeInfo, m_selectedLineGroup);

    if (m_showTypes) {
        m_signalVisualizerWidget -> getModel() -> applyColorToLineGroup(newTypeColor, m_selectedLineGroup);
    } else {
        m_signalVisualizerWidget -> getModel() -> applyColorToLineGroup(newDesignationColor, m_selectedLineGroup);
    }
    m_signalVisualizerWidget -> getModel() -> applyThicknessToLineGroup(newThickness, m_selectedLineGroup);

    updateLegend();
    deselectLine(m_selectedLineGroup);
    clearSelection();
    m_signalVisualizerWidget -> getModel() -> updateNetColors(m_showTypes);
    hideEditor();
}

void SignalVisualizerView::updateDesignationCombo() {
    QList<QString> newItems = m_signalVisualizerWidget -> getModel() -> getExtractedDesignations();

    m_signalDesignationCombo->clear();
    m_signalDesignationCombo->addItems(newItems);
}

void SignalVisualizerView::updateTypeCombo() {
    QList<QString> newItems = m_signalVisualizerWidget -> getModel() -> getExtractedCategories();

    m_signalTypeCombo->clear();
    m_signalTypeCombo->addItems(newItems);
}

void SignalVisualizerView::setCompLabelVisibility(bool visible) {
    for (ComponentOverlayTextItem* item : m_overlayItems) {
        item->setLabelVisible(visible);
    }
}

void SignalVisualizerView::setCompTextVisibility(bool visible) {
    for (ComponentOverlayTextItem* item : m_overlayItems) {
        item->setTextVisible(visible);
    }
}

void SignalVisualizerView::setCompPosDesignationVisibility(bool visible) {
    for (ComponentOverlayTextItem* item : m_overlayItems) {
        item->setPosDesignationVisible(visible);
    }
}

void SignalVisualizerView::displayConnecors(Circuit* circuit) {
    if (!circuit) return;

    const QList<Connector*>* connectors = circuit->conList();
    for (Connector* conn : *connectors) {
        if (conn) {
            QStringList pointList = conn->pointList();
            if (!pointList.isEmpty()) {
                QVector<QPointF> points;

                // Преобразование точек из строки в координаты
                for (int i = 0; i < pointList.size(); i += 2) {
                    points.append(QPointF(pointList[i].toDouble(), pointList[i + 1].toDouble()));
                }

                Pin* startPin = conn->startPin();
                Pin* endPin = conn->endPin();

                if (startPin && endPin) {
                    QColor lineColor = Qt::darkGreen;
                    QPen pen(lineColor, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    
                    Component* startComp = dynamic_cast<Component*>(startPin->parentItem());
                    Component* endComp = dynamic_cast<Component*>(endPin->parentItem());
                    if (!startComp || !endComp) {
                        qWarning() << "Parent is not a Component!";
                        continue;
                    }
                    QString startPinId = startPin -> pinId();
                    QString endPinId = endPin -> pinId();

                    QList<QGraphicsLineItem*> lineItems;

                    for (int i = 0; i < points.size() - 1; ++i) {
                        if (points[i] != points[i + 1]) {
                            bool duplicateLine = false;
                            for (auto& item : lineItems) {
                                if (item->line().p1() == points[i] && item->line().p2() == points[i + 1]) {
                                    duplicateLine = true;
                                    break;
                                }
                            }

                            if (!duplicateLine) {
                                QGraphicsLineItem* lineItem = m_signalVisualizerWidget->m_scene->addLine(QLineF(points[i], points[i + 1]), pen);
                                lineItem->setZValue(ZLevel::Lines);
                                lineItems.append(lineItem);
                            }
                        }
                    }

                    QPointF start = startPin->scenePos();
                    QPointF end = endPin->scenePos();
                    if (!pointList.isEmpty()) {
                        QPointF firstPoint(pointList[0].toDouble(), pointList[1].toDouble());
                        if (start != firstPoint) {
                            QGraphicsLineItem* lineItem = m_signalVisualizerWidget->m_scene->addLine(QLineF(start, firstPoint), pen);
                            lineItem->setZValue(ZLevel::Lines);
                            lineItems.append(lineItem);
                        }
                    }
                    if (pointList.size() > 2) {
                        QPointF lastPoint(pointList[pointList.size() - 2].toDouble(),
                                          pointList[pointList.size() - 1].toDouble());
                        if (end != lastPoint) {
                            QGraphicsLineItem* lineItem = m_signalVisualizerWidget->m_scene->addLine(QLineF(lastPoint, end), pen);
                            lineItem->setZValue(ZLevel::Lines);
                            lineItems.append(lineItem);
                        }
                    }
                    // Создание (обновление) карты соединений
                    m_signalVisualizerWidget -> getModel() -> updateConnectionsMap(startPin, endPin, lineItems);
                }
            }
        }
    }
}

void SignalVisualizerView::displayComponents(Circuit* circuit) {
    if (!circuit) return;

    const QList<Component*>* components = circuit->compList();
    for (Component* comp : *components) {
        if (comp) {
            QPointF scenePos = comp->scenePos();
            QRectF boundingRect = comp->mapRectToScene(comp->boundingRect());

            MainComponentProxyItem* proxyItem = new MainComponentProxyItem(comp, this);
            proxyItem->setZValue(ZLevel::Components);
            m_signalVisualizerWidget->m_scene->addItem(proxyItem);

            ComponentOverlayTextItem* overlayItem = new ComponentOverlayTextItem(comp, this);
            overlayItem->setZValue(ZLevel::Labels);
            m_signalVisualizerWidget->m_scene->addItem(overlayItem);
            m_overlayItems.append(overlayItem);
        }
    }
}

void SignalVisualizerView::displayNodes(Circuit* circuit) {
    if (!circuit) return;

    const QList<Node*>* nodes = circuit->nodeList();
    for (Node* node : *nodes) {
        NodeProxyItem* proxyItem = new NodeProxyItem(node);
        proxyItem->setZValue(ZLevel::Nodes);
        m_signalVisualizerWidget->m_scene->addItem(proxyItem);
    }
}
