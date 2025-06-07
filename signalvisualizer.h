#ifndef SIGNALVISUALIZER_H
#define SIGNALVISUALIZER_H

#include <QObject>
#include <QMap>
#include <QColor>
#include <QFileDialog>
#include <QMessageBox>
#include <QDataStream>
#include <QFile>
#include <QXmlStreamReader>
#include <algorithm>
#include "circuit.h"
#include "pin.h"
#include "rail.h"
#include "varsource.h"
#include "fixedvolt.h"
#include "battery.h"
#include "tunnel.h"
#include "signalvisualizerview.h"
#include "signalvisualizerwidget.h"

class SignalVisualizer : public QObject
{
    Q_OBJECT
    friend class SignalVisualizerView;
public:
    explicit SignalVisualizer(QObject *parent = nullptr);

    struct NetConnections {
        QList<QGraphicsLineItem*> lineList;
        QList<Pin*> pinList;
        QString designationInfo;
        QString typeInfo;
        QString designation;
        QString type;
        QColor lineColor;
        QColor designationColor;
        QColor typeColor;
        
        NetConnections() = default;
    
        NetConnections(QList<QGraphicsLineItem*> lineList,
                   QList<Pin*> pinList,
                   QString designationInfo = "",
                   QString typeInfo = "",
                   QString designation = "",
                   QString type = "",
                   QColor lineColor = QColor(),
                   QColor designationColor = Qt::darkGreen,
                   QColor typeColor = Qt::darkGreen)
        : lineList(lineList), pinList(pinList), designationInfo(designationInfo), typeInfo(typeInfo), designation(designation), type(type), lineColor(lineColor), designationColor(designationColor), typeColor(typeColor) {}
    };

    struct SignalAttributes {
        QString designation;
        QColor designationColor;
        QString designationInfo;
        QString type;
        QColor typeColor;
        QString typeInfo;
    };

    struct NetFlags {
        bool isSource = false;
        bool isRail = false;
        bool isFixedVolt = false;
        bool isVoltageSource = false;
        bool isBattPlus = false;
        bool isBattMinus = false;
        bool isGround = false;
        bool isDestination = false;
        bool isReset = false;
        bool isClear = false;
        bool isCS = false;
        bool isDC = false;
        bool isEN = false;
        bool isRW = false;
        bool isSCL = false;
        bool isSCLK = false;
        bool isSDA = false;
        bool isTx = false;
        bool isRx = false;
        bool isMISO = false;
        bool isMOSI = false;
        bool isDATA = false;
        bool isGPIO = false;
    };

    bool isSystemType(const QString& type) const;
    bool isSystemDesignationType(const QString& type) const;

    QList<QString> getExtractedDesignations() const;
    QList<QString> getExtractedCategories() const;
    QColor getLineColorByGroup(const QList<QGraphicsLineItem*>& lineGroup, bool isShowingTypes) const;
    QColor getDesignationColorByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QColor getTypeColorByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QList<QColor> getColorsByDesignation(const QString &designation) const;
    QList<QColor> getColorsByType(const QString &type) const;
    QString getTypeByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QString getDesignationByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QList<int> getThicknessesByDesignation(const QString &designation) const;
    QString getTypeInfoByLine(QGraphicsLineItem* lineItem) const;
    QString getDesignationInfoByLine(QGraphicsLineItem* lineItem) const;
    QString getDesignationInfoByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QString getTypeInfoByGroup(const QList<QGraphicsLineItem*>& lineGroup) const;
    QString getPositionalDesignation(const QString &type);
    QList<QGraphicsLineItem*> getGroupByLine(QGraphicsLineItem* lineItem) const;

    void removeDesignationForConnections(QString designation);
    void removeTypeForConnections(const QString& type);
    void resetConnectionsByGroup(const QList<QGraphicsLineItem*>& lineGroup);

    void setDesignationByGroup(const QString& designation, QList<QGraphicsLineItem*>& lineGroup);
    void setDesignationInfoByGroup(const QString& info, QList<QGraphicsLineItem*>& lineGroup);
    void setTypeByGroup(const QString& type, QList<QGraphicsLineItem*>& lineGroup);
    void setTypeInfoByGroup(const QString& info, QList<QGraphicsLineItem*>& lineGroup);
    void setDesignationLineColorByGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup);
    void setTypeLineColorByGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup);
    
    void applyColorToLineGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup);
    void applyThicknessToLineGroup(int thickness, const QList<QGraphicsLineItem*>& lineGroup);

    void updateNetColors(bool showCategories);
    void updateConnectionsMap(Pin* startPin, Pin* endPin, QList<QGraphicsLineItem*>& lineItems);
    
    void colorizeCircuit();

    QString toString();
    void loadFromString(const QString &xmlString);
    
signals:
    void comboUpdated();
    void legendUpdated();
    void colorizeFinished();
    void sceneUpdated();

private:
    QMap<QString, NetConnections> m_netConnections;
    
    void assignVoltageGradientColors();
    QString formatVoltage(double value);
    bool hasPin(const QString& pinId, const QString& name);
    bool matchRegex(const QString& pinId, const QString& pattern);

    void setDesignation(const QString& designation, NetConnections& net);
    void setType(const QString& type, NetConnections& net);
    void setDesignationLineColor(const QColor& color, NetConnections& net);
    void setTypeLineColor(const QColor& color, NetConnections& net);
    void setDesignationInfo(const QString& info, NetConnections& net);
    void setTypeInfo(const QString& info, NetConnections& net);

    void mergeDuplicateKeysNodes(QMap<QString, NetConnections>& mapNetConnections);
    QString findMatchingKeyForPin(const QString& pinId, QMap<QString, NetConnections>& mapNetConnections);
    QString findMatchingKeyForNodeConnection(const QString& pinId, QMap<QString, NetConnections>& mapNetConnections);

    QList<QString> m_systemTypes;
    QList<QString> m_systemDesignationsTypes;

    QMap<QString, std::function<void(NetFlags&, double&, QString&, Component*, const QString&)>> m_sourceHandlers;
    QMap<QString, std::function<void(NetFlags&, const QString&)>> m_destHandlers;
    QMap<QString, std::function<void(NetFlags&, QString&, Component*, const QString&)>> m_multiHandlers;
    QMap<QString, QPair<QString, int>> m_posDesignation;
    QMap<QString, QString> m_typeToGroup;

    void analyzePins(const NetConnections& net, NetFlags& flags, QString& designation);
    void determineSignalType(const NetFlags& flags, const QString& designation, SignalAttributes& attr);
    void initControlSignal(SignalAttributes& attr, const QString& name, const QString& info);
    void initDataSignal(SignalAttributes& attr, const QString& name, const QString& info);
    void applyLineAppearance(const SignalAttributes& attr, NetConnections& net);
};

#endif // SIGNALVISUALIZER_H
