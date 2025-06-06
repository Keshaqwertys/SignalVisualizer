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

#endif // SIGNALVISUALIZERGRAPHICSVIEW_H
