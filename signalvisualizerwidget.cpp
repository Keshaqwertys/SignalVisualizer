#include "signalvisualizerwidget.h"
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include "circuit.h"
#include <iostream>
#include "pin.h"
#include "connector.h"
#include "compbase.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include "comproperty.h"
#include "image.h"
#include "resistordip.h"
#include "keypad.h"
#include "subcircuit.h"
#include "mcu.h"
#include "rail.h"
#include "varsource.h"
#include "battery.h"
#include <algorithm>


SignalVisualizerWidget::SignalVisualizerWidget(QWidget *parent) : QWidget(parent)
{
    m_circuitInstance = Circuit::self();
    m_visualizerModel = new SignalVisualizer(this);
    connect(m_visualizerModel, &SignalVisualizer::sceneUpdated, this, &SignalVisualizerWidget::updateScene);
    setupUI();
    applyStyles();
}

void SignalVisualizerWidget::setupUI() {
    m_layout = new QVBoxLayout(this);
    m_menuBar = new QMenuBar(this);
    m_fileMenu = m_menuBar->addMenu(tr("Файл"));

    QAction *loadAction = new QAction(tr("Загрузить"), this);
    QAction *saveAction = new QAction(tr("Сохранить"), this);
    QAction *saveAsAction = new QAction(tr("Сохранить как..."), this);
    m_fileMenu->addAction(saveAction);
    m_fileMenu->addAction(saveAsAction);
    m_fileMenu->addAction(loadAction);
    connect(saveAction, &QAction::triggered, this, [this]() {
        this->saveConfig();
    });
    connect(loadAction, &QAction::triggered, this, [this]() {
        this->loadConfig();
    });
    connect(saveAsAction, &QAction::triggered, this, &SignalVisualizerWidget::saveAsConfig);

    m_layout->setMenuBar(m_menuBar);
    setWindowFlags(Qt::Window);

    m_label = new QLabel("Визуализация типов электрических сигналов");
    m_label->setAlignment(Qt::AlignCenter);

    m_scene = new QGraphicsScene(this);
    m_graphicsView = new SignalVisualizerView(this, parentWidget());
    m_graphicsView->setScene(m_scene);
    m_graphicsView->setMinimumSize(600, 400);

    m_viewMenu = m_menuBar->addMenu(tr("Отображение"));
    QAction *toggleViewAction = new QAction(
        m_graphicsView->isShowingTypes() ? tr("Показать обозначения") : tr("Показать типы сигналов"), this);
    connect(toggleViewAction, &QAction::triggered, this, [this, toggleViewAction]() {
        m_graphicsView->toggleDisplayModeExternal();
        toggleViewAction->setText(
            m_graphicsView->isShowingTypes() ? tr("Показать обозначения") : tr("Показать типы сигналов"));
    });
    m_viewMenu->addAction(toggleViewAction);

    m_helpMenu = m_menuBar->addMenu(tr("Помощь"));
    QAction *helpAction = new QAction(tr("Справка"), this);
    m_helpMenu->addAction(helpAction);
    connect(helpAction, &QAction::triggered, this, &SignalVisualizerWidget::showHelp);

    m_closeButton = new QPushButton("Закрыть");
    connect(m_closeButton, &QPushButton::clicked, this, &SignalVisualizerWidget::closeVisualizer);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_graphicsView);
    m_layout->addWidget(m_closeButton);

    setWindowTitle("Visualizer");
    setMinimumSize(600, 400);
}

void SignalVisualizerWidget::applyStyles() {
    m_label->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");
    m_closeButton->setStyleSheet(R"(
        QPushButton {
            background-color:rgb(202, 175, 206);
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color:rgb(158, 138, 161);
        }
    )");
}

void SignalVisualizerWidget::closeVisualizer() {
    this->close();
}

void SignalVisualizerWidget::saveConfig() {
if (m_currentFileName.isEmpty()) {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Сохранить файл"),
        "",
        tr("ColorScheme Files (*.cscfg)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (!fileName.isEmpty()) {
        saveConfig(fileName);
    }
} else {
    saveConfig(m_currentFileName);
}
}

void SignalVisualizerWidget::saveConfig(QString &fileName) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    if( !fileName.endsWith(".cscfg") ) fileName.append(".cscfg");
    QString config;
    if(m_visualizerModel) {
        config = m_visualizerModel -> toString();
    }
    QFile file( fileName );
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, "Ошибка сохранения",
                    tr("Не удалось записать файл %1:\n%2.")
                         .arg(fileName)
                         .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << config;
    file.close();

    QApplication::restoreOverrideCursor();
    QMessageBox::information(this, "Сохранение успешно", "Файл успешно сохранён!");
    m_currentFileName = fileName;
}

void SignalVisualizerWidget::saveAsConfig() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Сохранить как..."),
        "",
        tr("ColorScheme Files (*.cscfg)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (!fileName.isEmpty()) {
        saveConfig(fileName);
    }
}

void SignalVisualizerWidget::loadConfig() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Загрузить файл конфигурации"),
        "",
        tr("ColorScheme Files (*.cscfg);;Все файлы (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (!fileName.isEmpty()) {
        loadConfig(fileName);
    }
}

void SignalVisualizerWidget::loadConfig(const QString &fileName) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, tr("Ошибка загрузки"),
            tr("Не удалось открыть файл %1:\n%2.")
                .arg(fileName)
                .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QString xmlContent = in.readAll();
    file.close();

    m_visualizerModel->loadFromString(xmlContent);
    QApplication::restoreOverrideCursor();
    QMessageBox::information(this, tr("Загрузка завершена"),
        tr("Конфигурация успешно загружена из\n%1").arg(fileName));

    m_currentFileName = fileName;
    m_graphicsView->clearSelection();
    m_graphicsView->hideEditor();
}

void SignalVisualizerWidget::showHelp() {
    QMessageBox::information(
        this,
        tr("Справка"),
        tr("Эта утилита предназначена для визуализации электрических сигналов на схеме.\n\n"
           "Основные возможности:\n"
           "- Автоматическая типизация сигналов\n"
           "- Ручная настройка цветов\n"
           "- Экспорт/импорт цветовых схем\n"
           "- Подсветка соединений и отображение меток\n\n"
           "Используйте левую кнопку мыши для выделения линий и доступа к настройке параметров отображения.")
    );
}

void SignalVisualizerWidget::updateScene() {
    m_scene -> update();
}

SignalVisualizerWidget::~SignalVisualizerWidget(){
    delete m_scene;
}
