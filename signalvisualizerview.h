#ifndef SIGNALVISUALIZERGRAPHICSVIEW_H
#define SIGNALVISUALIZERGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QVector>
#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QFormLayout>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QTextEdit>
#include <QtMath>
#include <unordered_set>
#include "circuit.h"
#include "pin.h"
#include "mcu.h"
#include "chip.h"
#include "iocomponent.h"
#include "iopin.h"
#include "e-node.h"
#include "label.h"
#include "tunnel.h"
#include "subcircuit.h"
#include "plotbase.h"
#include "logiccomponent.h"
#include "node.h"

class SignalVisualizerWidget;
class ComponentOverlayTextItem;

inline uint qHash(const QColor &color, uint seed = 0) noexcept {
    return qHash(color.rgba(), seed);
}

namespace ZLevel {
    constexpr double Background = 0;
    constexpr double Lines = 10;
    constexpr double Components = 20;
    constexpr double Nodes = 25;
    constexpr double Labels = 30;
}

class SignalVisualizerView : public QGraphicsView {
    Q_OBJECT
    friend class SignalVisualizer;
    friend class SignalVisualizerWidget;
    friend class MainComponentProxyItem;
    friend class SubComponentProxyItem;
    friend class ComponentOverlayTextItem;
    friend class PinProxyItem;

    struct LegendItem {
        QColor color;
        QString label;
    };

public:
    explicit SignalVisualizerView(SignalVisualizerWidget* signalVisualizerWidget, QWidget *parent = nullptr);
    const QList<QGraphicsLineItem*>& getSelectedLineGroup() const;
    bool isShowingTypes() const { return m_showTypes; }
    void toggleDisplayModeExternal() { toggleDisplayMode(!m_showTypes); }

public slots:
    void toggleDisplayMode(bool showCategories);
    void setCompLabelVisibility(bool visible);
    void setCompTextVisibility(bool visible);
    void updateDesignationCombo();
    void updateTypeCombo();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void createEditor();
    void applyStyles();
    void setLegend(const QVector<LegendItem> &legendItems);

    void updateOverlayPosition();
    void updateEditorOverlayPosition();
    void updateLegendOverlay();
    void updateLegend();
    void updateCheckboxOverlayPosition();

    void toggleCompLabelVisibility(int state);
    void toggleCompTextVisibility(int state);

    void selectLine(QList<QGraphicsLineItem*>& lineGroup);
    void deselectLine(QList<QGraphicsLineItem*>& lineGroup);

    void signalDesignationsManager();
    void signalTypesManager();

    void updateColorComboForDesignation(const QString &designation);
    void updateColorComboForType(const QString &designation);
    void updateThicknessSpinForDesignation(const QString &designation);

    void fillItemsForColorComboBox(QComboBox* comboBox);
    void resetSelection();
    void clearSelection();

    void hideEditor();
    void applyChanges();

    QStringList getCurrentDesignations(QComboBox* combo);

    void displayComponents(Circuit* circuit);
    void displayConnecors(Circuit* circuit);
    void displayNodes(Circuit* circuit);

    Circuit* m_circuitInstance;
    QVector<LegendItem> m_legendItems;

    SignalVisualizerWidget* m_signalVisualizerWidget;

    QVBoxLayout *m_layout;
    QVBoxLayout *m_checkboxLayout;
    QHBoxLayout *m_buttonLayout;
    QFormLayout *m_lineEditLayout;

    QPushButton *m_manageDesignationsButton;
    QPushButton *m_manageTypeButton;
    QPushButton *m_applyButton;
    QPushButton *m_resetButton;

    QComboBox* m_signalDesignationCombo;
    QComboBox* m_signalTypeCombo;
    QComboBox* m_designationColorCombo;
    QComboBox* m_typeColorCombo;
    QSpinBox* m_thicknessSpin;

    bool m_showTypes = false;
    const qreal m_scaleFactor = 1.15;
    bool m_isPanning = false;
    QPoint m_lastMousePosition;

    QWidget *m_lineEditOverlay;
    QWidget *m_legendOverlay;
    QWidget *m_checkboxOverlay;
    QCheckBox *m_hideCompLabelCheckbox;
    QCheckBox *m_hideCompTextCheckbox;

    QLabel *m_tooltipLabel;
    QGraphicsLineItem *m_hoveredItem;

    QTextEdit* m_designationInfoEdit;
    QTextEdit* m_typeInfoEdit;

    QGraphicsLineItem* m_selectedLineItem = nullptr;
    QList<QGraphicsLineItem*> m_selectedLineGroup;
    
    QList<ComponentOverlayTextItem*> m_overlayItems;
};

class PinProxyItem : public QObject, public QGraphicsItem {
    public:
        PinProxyItem(Pin* pin) : m_pin(pin) {
            // Подписываемся на сигнал уничтожения компонента
            connect(m_pin, &Component::destroyed, this, [this]() {
                this->deleteLater(); // Удаляем прокси при удалении компонента
            });

            setPos(pin->pos());
            setRotation(pin->rotation());
            setFlag(QGraphicsItem::ItemStacksBehindParent, true);
        }
    
        QRectF boundingRect() const override {
            return m_pin->boundingRect();
        }
    
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
            if (!m_pin) return;
            m_pin->paint(painter, option, widget);
        }
    
    private:
        Pin* m_pin;
    };

class MainComponentProxyItem :public QObject, public QGraphicsItem {
    public:
        MainComponentProxyItem(Component* comp) : m_component(comp) {
            connect(m_component, &Component::destroyed, this, [this]() {
                this->deleteLater();
            });

            setPos(m_component->scenePos());
            setRotation(m_component->rotation());
            if (m_component->itemType() != "KeyPad") {
                setTransform(QTransform::fromScale(comp->hflip(), comp->vflip()));
            }
            if (m_component->itemType() != "Subcircuit") {
                createPinItems();
            }
        }
        
        QRectF boundingRect() const override {
            return m_component->boundingRect();
        }

        void createPinItems() {
            for (Pin* pin : m_component->getPins()) {
                if (!pin) continue;
                
                auto* pinItem = new PinProxyItem(pin);
                pinItem->setParentItem(this);
                m_pinItems.push_back(pinItem);

                QGraphicsSimpleTextItem* origLabel = pin->getLabelItem();
                if (origLabel) {
                    m_labelCopy = copyLabelItem(origLabel, this);
                }
            }
            Mcu* mcu = dynamic_cast<Mcu*>(m_component);
            if (mcu) {
                for (Pin* pin : mcu->getPinList()) {
                    if (!pin) continue;

                    auto* pinItem = new PinProxyItem(pin);
                    pinItem->setParentItem(this);
                    m_pinItems.push_back(pinItem);

                    if (QGraphicsSimpleTextItem* origLabel = pin->getLabelItem()) {
                        QGraphicsSimpleTextItem* labelCopy = copyLabelItem(origLabel, this);
                        labelCopy->setZValue(500);
                    }
                }
            }
            IoComponent* ioComp = dynamic_cast<IoComponent*>(m_component);
            if (ioComp) {
                std::vector<IoPin*> combinedPins;
                combinedPins.reserve(
                    ioComp->inPins().size() + ioComp->outPins().size() + ioComp->otherPins().size()
                );

                combinedPins.insert(combinedPins.end(), ioComp->inPins().begin(), ioComp->inPins().end());
                combinedPins.insert(combinedPins.end(), ioComp->outPins().begin(), ioComp->outPins().end());
                combinedPins.insert(combinedPins.end(), ioComp->otherPins().begin(), ioComp->otherPins().end());

                LogicComponent* logicComp = dynamic_cast<LogicComponent*>(m_component);
                if (logicComp) {
                    if (IoPin* oe = const_cast<IoPin*>(logicComp->oePin())) {
                        combinedPins.push_back(oe);
                    }
                }

                for (IoPin* pin : combinedPins) {
                    if (!pin) continue;

                    auto* pinItem = new PinProxyItem(pin);
                    pinItem->setParentItem(this);
                    m_pinItems.push_back(pinItem);

                    if (QGraphicsSimpleTextItem* origLabel = pin->getLabelItem()) {
                        QGraphicsSimpleTextItem* labelCopy = copyLabelItem(origLabel, this);
                        labelCopy->setZValue(500);
                    }
                }
            }
        }
        
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
            if (!m_component) return;
            m_component->paint(painter, option, widget);
        }
        
    private:
        QGraphicsSimpleTextItem* copyLabelItem(QGraphicsSimpleTextItem* origLabel, QGraphicsItem* parent = nullptr) {
            if (!origLabel) return nullptr;
    
            QGraphicsSimpleTextItem* copy = new QGraphicsSimpleTextItem(origLabel->text(), parent);
            copy->setFont(origLabel->font());
            
            copy->setBrush(origLabel->brush());
            copy->setPen(origLabel->pen());
            copy->setPos(origLabel->pos());
            copy->setRotation(origLabel->rotation());
            copy->setTransform(origLabel->transform());
    
            return copy;
        }
    
        QString m_id;
        Component* m_component;
        QList<QGraphicsTextItem*> m_labelItems;
        QList<QGraphicsTextItem*> m_textItems;
        QGraphicsSimpleTextItem* m_labelCopy = nullptr;
        std::vector<PinProxyItem*> m_pinItems;
        };
    
    class SubComponentProxyItem : public QGraphicsItem {
        public:
            SubComponentProxyItem(Component* comp) : m_component(comp) {
                setPos(comp->scenePos());
                setRotation(comp->rotation());
                if (comp->itemType() != "KeyPad") {
                    setTransform(QTransform::fromScale(comp->hflip(), comp->vflip()));
                }
            }
        
            QRectF boundingRect() const override {
                return m_component->boundingRect();
            }
        
            void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
                if (!m_component) return;
                m_component->paint(painter, option, widget);
            }
        
        private:
            QString id;
            Component* m_component;
            QList<QGraphicsTextItem*> labelItems;
            QList<QGraphicsTextItem*> textItems;
            
    };
    
    class ComponentOverlayTextItem : public QGraphicsItem {
        public:
            ComponentOverlayTextItem(Component* comp, QGraphicsItem* parent = nullptr)
                : QGraphicsItem(parent), m_component(comp) {
                
                // Label (ID)
                if (Label* idLabel = comp->getIdLabel()) {
                    if (idLabel->isVisible()){
                        m_idTextItem = new Label();
                        m_idTextItem->setComponent(comp);
                        m_idTextItem->setPlainText(idLabel->toPlainText());
                        m_idTextItem->setFont(idLabel->font());
                        m_idTextItem->setDefaultTextColor(Qt::black);
                        m_idTextItem->setParentItem(this);
                        m_textItems.append(m_idTextItem);
                    }
                }
                
                // Label (значение)
                if (Label* label = comp->getValLabel()) {
                    if (label->isVisible()){
                        m_valLabel = new Label();
                        m_valLabel->setPlainText(label->toPlainText());
                        m_valLabel->setFont(label->font());
                        m_valLabel->setDefaultTextColor(Qt::darkRed);
                        m_valLabel->setParentItem(this);
                        m_valLabel->setAcceptedMouseButtons(Qt::NoButton);
                        m_labelItems.append(m_valLabel);
                    }
                }
        
                updateTextPosition();
            }
        
            void updateTextPosition() {
                QRectF compRect = m_component->boundingRect();
                QPointF compPos = m_component->scenePos();
        
                if (m_idTextItem)
                    m_idTextItem->setPos(compPos + QPointF(compRect.width() / 2 - m_idTextItem->boundingRect().width() / 2,
                                                             compRect.top() - m_idTextItem->boundingRect().top() - 15));

                if (m_valLabel)
                    m_valLabel->setPos(compPos + QPointF(5, 0));
            }
        
            QRectF boundingRect() const override {
                return childrenBoundingRect();
            }
        
            void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {
            }
        
            void setTextVisible(bool visible) {
                for (auto* textItem : m_textItems) {
                    if (textItem)
                        textItem->setVisible(visible);
                }
            }
        
            void setLabelVisible(bool visible) {
                for (auto* labelItem : m_labelItems) {
                    if (labelItem)
                        labelItem->setVisible(visible);
                }
            }
        
            const QList<QGraphicsTextItem*>& getTextItems() const { return m_textItems; }
            const QList<QGraphicsTextItem*>& getLabelItems() const { return m_labelItems; }
        
        private:
            Component* m_component;
            Label* m_idTextItem = nullptr;
            Label* m_valLabel = nullptr;
            QList<QGraphicsTextItem*> m_textItems;
            QList<QGraphicsTextItem*> m_labelItems;
        };

class NodeProxyItem : public QGraphicsItem {
public:
    NodeProxyItem(Node* node) : m_node(node) {
        setPos(node->scenePos());
        setRotation(node->rotation());
    }

    QRectF boundingRect() const override {
        return m_node->boundingRect();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        if (!m_node) return;
        m_node->paint(painter, option, widget);
    }

private:
    Node* m_node;
};

#endif // SIGNALVISUALIZERGRAPHICSVIEW_H
