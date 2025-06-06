#include "proxyitem.h"
#include "pin.h"
#include "mcu.h"
#include "iocomponent.h"
#include "iopin.h"
#include "logiccomponent.h"
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include "signalvisualizerview.h"
#include <QGraphicsSimpleTextItem>
#include <unordered_set>
#include "circuit.h"
#include "chip.h"
#include "e-node.h"
#include "label.h"
#include "tunnel.h"
#include "subcircuit.h"
#include "plotbase.h"
#include "node.h"

MainComponentProxyItem::MainComponentProxyItem(Component* comp)
    : m_component(comp) {
    connect(m_component, &QObject::destroyed, this, [this]() {
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

QRectF MainComponentProxyItem::boundingRect() const {
    return m_component->boundingRect();
}

void MainComponentProxyItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (!m_component) return;
    m_component->paint(painter, option, widget);
}

QGraphicsSimpleTextItem* MainComponentProxyItem::copyLabelItem(QGraphicsSimpleTextItem* origLabel, QGraphicsItem* parent) {
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

void MainComponentProxyItem::createPinItems() {
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

PinProxyItem::PinProxyItem(Pin* pin)
    : m_pin(pin)
{
    // Подписываемся на сигнал уничтожения компонента
    connect(m_pin, &Component::destroyed, this, [this]() {
        this->deleteLater(); // Удаляем прокси при удалении компонента
    });

    setPos(pin->pos());
    setRotation(pin->rotation());
    setFlag(QGraphicsItem::ItemStacksBehindParent, true);
}

QRectF PinProxyItem::boundingRect() const {
    return m_pin->boundingRect();
}

void PinProxyItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (!m_pin) return;
    m_pin->paint(painter, option, widget);
}

SubComponentProxyItem::SubComponentProxyItem(Component* comp)
    : m_component(comp)
{
    setPos(comp->scenePos());
    setRotation(comp->rotation());

    if (comp->itemType() != "KeyPad") {
        setTransform(QTransform::fromScale(comp->hflip(), comp->vflip()));
    }
}

QRectF SubComponentProxyItem::boundingRect() const {
    return m_component->boundingRect();
}

void SubComponentProxyItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (!m_component) return;
    m_component->paint(painter, option, widget);
}

ComponentOverlayTextItem::ComponentOverlayTextItem(Component* comp, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_component(comp)
{
    // Label (ID)
    if (Label* idLabel = comp->getIdLabel()) {
        if (idLabel->isVisible()) {
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
        if (label->isVisible()) {
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

void ComponentOverlayTextItem::updateTextPosition() {
    QRectF compRect = m_component->boundingRect();
    QPointF compPos = m_component->scenePos();

    if (m_idTextItem) {
        m_idTextItem->setPos(compPos + QPointF(compRect.width() / 2 - m_idTextItem->boundingRect().width() / 2,
                                               compRect.top() - m_idTextItem->boundingRect().top() - 15));
    }

    if (m_valLabel) {
        m_valLabel->setPos(compPos + QPointF(5, 0));
    }
}

QRectF ComponentOverlayTextItem::boundingRect() const {
    return childrenBoundingRect();
}

void ComponentOverlayTextItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) {
    // Пустая реализация
}

void ComponentOverlayTextItem::setTextVisible(bool visible) {
    for (auto* textItem : m_textItems) {
        if (textItem)
            textItem->setVisible(visible);
    }
}

void ComponentOverlayTextItem::setLabelVisible(bool visible) {
    for (auto* labelItem : m_labelItems) {
        if (labelItem)
            labelItem->setVisible(visible);
    }
}

const QList<QGraphicsTextItem*>& ComponentOverlayTextItem::getTextItems() const {
    return m_textItems;
}

const QList<QGraphicsTextItem*>& ComponentOverlayTextItem::getLabelItems() const {
    return m_labelItems;
}

NodeProxyItem::NodeProxyItem(Node* node)
    : m_node(node) {
    setPos(node->scenePos());
    setRotation(node->rotation());
}

QRectF NodeProxyItem::boundingRect() const {
    return m_node->boundingRect();
}

void NodeProxyItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (!m_node) return;
    m_node->paint(painter, option, widget);
}
