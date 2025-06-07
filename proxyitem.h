#ifndef PROXYITEM_H
#define PROXYITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <vector>
#include <QList>

class Component;
class SignalVisualizerView;
class Pin;
class Node;
class Label;
class QGraphicsSimpleTextItem;
class PinProxyItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class MainComponentProxyItem : public QObject, public QGraphicsItem {
    Q_OBJECT
public:
    MainComponentProxyItem(Component* comp, SignalVisualizerView* view);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void createPinItems();
    QGraphicsSimpleTextItem* copyLabelItem(QGraphicsSimpleTextItem* origLabel, QGraphicsItem* parent = nullptr);

    QString m_id;
    Component* m_component;
    SignalVisualizerView* m_visualizerView;
    QList<QGraphicsTextItem*> m_labelItems;
    QList<QGraphicsTextItem*> m_textItems;
    QGraphicsSimpleTextItem* m_labelCopy = nullptr;
    std::vector<PinProxyItem*> m_pinItems;
};

class PinProxyItem : public QObject, public QGraphicsItem {
    Q_OBJECT

public:
    explicit PinProxyItem(Pin* pin);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    Pin* m_pin;
};

class SubComponentProxyItem : public QGraphicsItem {
public:
    explicit SubComponentProxyItem(Component* comp);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QString id;
    Component* m_component;
    QList<QGraphicsTextItem*> labelItems;
    QList<QGraphicsTextItem*> textItems;
};

class ComponentOverlayTextItem : public QGraphicsItem {
public:
    ComponentOverlayTextItem(Component* comp, SignalVisualizerView* view, QGraphicsItem* parent = nullptr);

    void updateTextPosition();
    void setTextVisible(bool visible);
    void setLabelVisible(bool visible);
    void setPosDesignationVisible(bool visible);

    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

    const QList<QGraphicsTextItem*>& getTextItems() const;
    const QList<QGraphicsTextItem*>& getLabelItems() const;
    const QList<QGraphicsTextItem*>& getPosDesignationslItems() const;

private:
    Component* m_component;
    SignalVisualizerView* m_visualizerView;
    Label* m_idTextItem = nullptr;
    Label* m_valLabel = nullptr;
    Label* m_posDesignationItem = nullptr;

    QList<QGraphicsTextItem*> m_textItems;
    QList<QGraphicsTextItem*> m_labelItems;
    QList<QGraphicsTextItem*> m_posDesignationItems;
};

class NodeProxyItem : public QGraphicsItem {
public:
    explicit NodeProxyItem(Node* node);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    Node* m_node;
};

#endif // PROXYITEM_H
