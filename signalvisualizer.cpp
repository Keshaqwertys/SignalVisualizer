#include "signalvisualizer.h"

SignalVisualizer::SignalVisualizer(QObject *parent)
    : QObject(parent),
    m_systemTypes{"Power", "Control Signals", "Data Signals", "GPIO"},
    m_systemDesignationsTypes{"Power"},
    m_sourceHandlers{
        {"Rail", [this](NetFlags& f, double& v, QString& d, Component* c, const QString&) {
            if (auto r = dynamic_cast<Rail*>(c)) {
                f.isSource = f.isRail = true;
                v = r->volt();
                d = formatVoltage(v) + "V";
            }
        }},
        {"Fixed Voltage", [this](NetFlags& f, double& v, QString& d, Component* c, const QString&) {
            if (auto fv = dynamic_cast<FixedVolt*>(c)) {
                f.isSource = f.isRail = true;
                v = fv->volt();
                d = formatVoltage(v) + "V";
            }
        }},
        {"Battery", [this](NetFlags& f, double& v, QString& d, Component* c, const QString& pid) {
            if (auto b = dynamic_cast<Battery*>(c)) {
                v = b->volt();
                bool isPlus = pid.contains("lPin", Qt::CaseInsensitive);
                if (isPlus) {
                    f.isBattPlus = f.isSource = true;
                    d = formatVoltage(v) + "VBATT+";
                } else {
                    f.isBattMinus = f.isSource = true;
                    d = formatVoltage(v) + "VBATT-";
                }
                if (!d.isEmpty()) d.remove(0, 1);
            }
        }},
        {"Ground", [this](NetFlags& f, double& /*v*/, QString& /*d*/, Component* /*c*/, const QString& /*pid*/) {
            f.isGround = f.isSource = true;
        }}
    },
    m_destHandlers {
        {"Hd44780", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinRS", Qt::CaseInsensitive)) f.isDC = f.isDestination = true;
            else if (pid.contains("PinRW", Qt::CaseInsensitive)) f.isRW = f.isDestination = true;
            else if (pid.contains("PinEn", Qt::CaseInsensitive)) f.isEN = f.isDestination = true;
            else if (pid.contains("dataPin", Qt::CaseInsensitive)) f.isDATA = f.isDestination = true;
        }},
        {"Aip31068_i2c", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinSCL", Qt::CaseInsensitive)) f.isSCL = f.isDestination = true;
            else if (pid.contains("PinSDA", Qt::CaseInsensitive)) f.isSDA = f.isDestination = true;
        }},
        {"Pcd8544", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinRst", Qt::CaseInsensitive)) f.isReset = f.isDestination = true;
            else if (pid.contains("PinScl", Qt::CaseInsensitive)) f.isSCLK = f.isDestination = true;
            else if (pid.contains("PinSi", Qt::CaseInsensitive)) f.isMOSI = f.isDestination = true;
            else if (pid.contains("PinCs", Qt::CaseInsensitive)) f.isCS = f.isDestination = true;
            else if (pid.contains("PinDc", Qt::CaseInsensitive)) f.isDC = f.isDestination = true;
        }},
        {"Ks0108", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinRst", Qt::CaseInsensitive)) f.isReset = f.isDestination = true;
            else if (pid.contains("PinRW", Qt::CaseInsensitive)) f.isRW = f.isDestination = true;
            else if (pid.contains("PinEn", Qt::CaseInsensitive)) f.isEN = f.isDestination = true;
            else if (pid.contains("PinDc", Qt::CaseInsensitive)) f.isDC = f.isDestination = true;
            else if (pid.contains("PinCs", Qt::CaseInsensitive)) f.isCS = f.isDestination = true;
            else if (pid.contains("dataPin", Qt::CaseInsensitive)) f.isDATA = f.isDestination = true;
        }},
        {"Ssd1306", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinSck", Qt::CaseInsensitive)) f.isSCL = f.isDestination = true;
            else if (pid.contains("PinSda", Qt::CaseInsensitive)) f.isSDA = f.isDestination = true;
        }},
        {"Ili9341", [](NetFlags& f, const QString& pid) {
            if (pid.contains("PinRst", Qt::CaseInsensitive)) f.isReset = f.isDestination = true;
            else if (pid.contains("PinSck", Qt::CaseInsensitive)) f.isSCLK = f.isDestination = true;
            else if (pid.contains("PinMosi", Qt::CaseInsensitive)) f.isMOSI = f.isDestination = true;
            else if (pid.contains("PinCs", Qt::CaseInsensitive)) f.isCS = f.isDestination = true;
            else if (pid.contains("PinDc", Qt::CaseInsensitive)) f.isDC = f.isDestination = true;
        }},
        {"Dht22", [](NetFlags& f, const QString& pid) {
            f.isDATA = f.isDestination = true;
        }},
        {"Seven Segment", [](NetFlags& f, const QString& pid) {
            f.isDATA = f.isDestination = true;
        }}
    },
    m_multiHandlers {
        {"MCU", [this](NetFlags& f, QString& d, Component* c, const QString& pid) {
            QRegularExpression regex("^[\\w\\s]+-\\d+-PORT([A-Z][a-zA-Z0-9]+)$", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = regex.match(pid);

            if (pid.contains("MCLR", Qt::CaseInsensitive)) f.isClear = f.isDestination = true;
            else if (match.hasMatch()) {
                QString portValue = match.captured(1);
                if(!matchRegex(portValue,"^V\\d+$")) {
                    f.isGPIO = true;
                    if (d.isEmpty()) {
                        d = "GPIO_" + portValue;
                    } else if (d.contains("GPIO", Qt::CaseInsensitive)) {
                        d = d + "," + portValue;
                    }
                }
            }
        }},
        
        {"I2CToParallel", [this](NetFlags& f, QString& d, Component* c, const QString& pid) {
            if (matchRegex(pid,"^I2C\\s*to\\s*Parallel-\\d+-in0$")) f.isSDA = f.isDestination = true;
            else if (matchRegex(pid,"^I2C\\s*to\\s*Parallel-\\d+-in1$")) f.isSCL = f.isDestination = true;
        }},
        {"SerialPort", [this](NetFlags& f, QString& d, Component* c, const QString& pid) {
            if (matchRegex(pid,"^SerialPort-\\d+-pin0$")) f.isTx = f.isSource = true;
            else if (matchRegex(pid,"^SerialPort-\\d+-pin1$")) f.isRx = f.isDestination = true;
        }},
        {"Esp01", [this](NetFlags& f, QString& d, Component* c, const QString& pid) {
            if (matchRegex(pid,"^Esp01-\\d+-pin0$")) f.isTx = f.isSource = true;
            else if (matchRegex(pid,"^Esp01-\\d+-pin1$")) f.isRx = f.isDestination = true;
        }}
    },
    m_posDesignation {
        {"AudioOutGroup", qMakePair(QString("BA"), 1)},
        {"CapacitorGroup", qMakePair(QString("C"), 1)},
        {"DigitalDevicesAndMicrochipsGroup", qMakePair(QString("D"), 1)},
        {"PowerSourceGroup", qMakePair(QString("G"), 1)},
        {"BatteryGroup", qMakePair(QString("GB"), 1)},
        {"DisplayGroup", qMakePair(QString("HG"), 1)},
        {"LedGroup", qMakePair(QString("HL"), 1)},
        {"RelayGroup", qMakePair(QString("K"), 1)},
        {"InductorGroup", qMakePair(QString("L"), 1)},
        {"LAnalizerGroup", qMakePair(QString("LA"), 1)},
        {"OscopeGroup", qMakePair(QString("O"), 1)},
        {"AmperimeterGroup", qMakePair(QString("PA"), 1)},
        {"FreqMeterGroup", qMakePair(QString("PF"), 1)},
        {"VoltimeterGroup", qMakePair(QString("PV"), 1)},
        {"ResistorGroup", qMakePair(QString("R"), 1)},
        {"PotentiometerGroup", qMakePair(QString("RP"), 1)},
        {"VarResistorGroup", qMakePair(QString("RU"), 1)},
        {"SwitchGroup", qMakePair(QString("S"), 1)},
        {"ButtonGroup", qMakePair(QString("SB"), 1)},
        {"KeyPadGroup", qMakePair(QString("XS"), 1)}
    },
    m_typeToGroup {
        {"AudioOut", "AudioOutGroup"},
        {"Capacitor", "CapacitorGroup"},
        {"MCU", "DigitalDevicesAndMicrochipsGroup"},
        {"Subcircuit", "DigitalDevicesAndMicrochipsGroup"},
        {"I2CToParallel", "DigitalDevicesAndMicrochipsGroup"},
        {"BcdTo7S", "DigitalDevicesAndMicrochipsGroup"},
        {"Rail", "PowerSourceGroup"},
        {"Fixed Voltage", "PowerSourceGroup"},
        {"Voltage Source", "PowerSourceGroup"},
        {"Battery", "BatteryGroup"},
        {"Led", "LedGroup"},
        {"LedRgb", "LedGroup"},
        {"LedBar", "LedGroup"},
        {"LedMatrix", "LedGroup"},
        {"Max72xx_matrix", "LedGroup"},
        {"WS2812", "LedGroup"},
        {"Hd44780", "DisplayGroup"},
        {"Aip31068_i2c", "DisplayGroup"},
        {"Pcd8544", "DisplayGroup"},
        {"Ks0108", "DisplayGroup"},
        {"Ssd1306", "DisplayGroup"},
        {"Ili9341", "DisplayGroup"},
        {"RelaySPST", "RelayGroup"},
        {"Inductor", "InductorGroup"},
        {"LAnalizer", "LAnalizerGroup"},
        {"Oscope", "OscopeGroup"},
        {"Amperimeter", "AmperimeterGroup"},
        {"FreqMeter", "FreqMeterGroup"},
        {"Voltimeter", "VoltimeterGroup"},
        {"Resistor", "ResistorGroup"},
        {"ResistorDip", "ResistorGroup"},
        {"Potentiometer", "PotentiometerGroup"},
        {"VarResistor", "VarResistorGroup"},
        {"SwitchDip","SwitchGroup"},
        {"Switch", "SwitchGroup"},
        {"Push", "ButtonGroup"},
        {"KeyPad", "KeyPadGroup"}
    }
{
    // Конструктор
}

bool SignalVisualizer::isSystemType(const QString& type) const {
    return m_systemTypes.contains(type);
}

bool SignalVisualizer::isSystemDesignationType(const QString& type) const {
    return m_systemDesignationsTypes.contains(type);
}

void SignalVisualizer::colorizeCircuit() {
    for (auto& net : m_netConnections) {
        NetFlags flags;
        QString designation;
        analyzePins(net, flags, designation);

        SignalAttributes attr;
        determineSignalType(flags, designation, attr);

        applyLineAppearance(attr, net);
    }
    assignVoltageGradientColors();
    emit comboUpdated();
    emit colorizeFinished();
}

void SignalVisualizer::analyzePins(const NetConnections& net, NetFlags& flags, QString& designation) {
    for (Pin* pin : net.pinList) {
        if (!pin) continue;

        Component* comp = dynamic_cast<Component*>(pin->parentItem());
        if (!comp) continue;

        const QString compType = comp->itemType();
        const QString pinId = pin->pinId();
        double value = 0.0;

        if (m_sourceHandlers.contains(compType)) {
            m_sourceHandlers[compType](flags, value, designation, comp, pinId);
        }

        if (m_destHandlers.contains(compType)) {
            m_destHandlers[compType](flags, pinId);
        }

        if (m_multiHandlers.contains(compType)) {
            m_multiHandlers[compType](flags, designation, comp, pinId);
        }
    }
}

void SignalVisualizer::determineSignalType(const NetFlags& flags, const QString& designation, SignalAttributes& attr) {
    if (flags.isDestination) {
        if (flags.isReset)       initControlSignal(attr, "RST", "Сброс устройства");
        else if (flags.isClear)  initControlSignal(attr, "CLR", "Очистка (обнуление) регистра или устройства");
        else if (flags.isCS)     initControlSignal(attr, "CS", "Выбор устройства");
        else if (flags.isDC)     initControlSignal(attr, "DC", "Выбор данные/команды");
        else if (flags.isEN)     initControlSignal(attr, "EN", "Разрешение работы");
        else if (flags.isRW)     initControlSignal(attr, "RW", "Выбор чтение/запись");
        else if (flags.isSCL)    initControlSignal(attr, "SCL", "Синхронизация в интерфейсе I²C");
        else if (flags.isSCLK)   initControlSignal(attr, "SCLK", "Синхронизация в интерфейсе SPI");
        else if (flags.isMISO)   initDataSignal(attr, "MISO", "Передача данных от ведомого к ведущему в интерфейсе SPI");
        else if (flags.isMOSI)   initDataSignal(attr, "MOSI", "Передача данных от ведущего к ведомому в интерфейсе SPI");
        else if (flags.isSDA)    initDataSignal(attr, "SDA", "Данные в интерфейсе I²C");
        else if (flags.isRx)     initDataSignal(attr, "RX", "Приём данных в интерфейсе UART");
        else if (flags.isDATA)   initDataSignal(attr, "DATA", "Передача данных");
    } else if (flags.isSource) {
        if (flags.isRail || flags.isFixedVolt || flags.isVoltageSource || flags.isBattPlus) {
            attr.designation = designation;
            attr.designationColor = Qt::red;
            attr.designationInfo = "Питающее напряжение";
            attr.type = "Power";
            attr.typeColor = Qt::red;
            attr.typeInfo = "Обеспечивают подачу энергии, общий опорный потенциал и логических уровней";

        } else if (flags.isBattMinus) {
            attr.designation = designation;
            attr.designationColor = Qt::black;
            attr.designationInfo = "Линия подключенная к отрицательному выводу батареи";
            attr.type = "Power";
            attr.typeColor = Qt::red;
            attr.typeInfo = "Обеспечивают подачу энергии, общий опорный потенциал и логических уровней";

        } else if (flags.isGround) {
            attr.designation = "GND";
            attr.designationColor = Qt::black;
            attr.designationInfo = "Общий провод";
            attr.type = "Power";
            attr.typeColor = Qt::red;
            attr.typeInfo = "Обеспечивают подачу энергии, общий опорный потенциал и логических уровней";
        } else if (flags.isTx) {
            initDataSignal(attr, "TX", "Передача данных в интерфейсе UART");
        }
    } else if (flags.isGPIO) {
        attr.designation = designation;
        attr.designationColor = Qt::green;
        attr.designationInfo = "Линия, подключенная к порту общего назначения,\nкоторый может быть сконфигурирован как вход или выход";
        attr.type = "GPIO";
        attr.typeColor = Qt::green;
        attr.typeInfo = "Сигналы общего назначения, которые по необходимости могут быть сконфигурированы как входы или выходы,\nа также как аналоговые входы, выходы для формирования широтно-импульсной модуляции\nили линии внешних прерываний при соответствующей аппаратной поддержке";
    }  else {
        attr.designation = "";
        attr.designationColor = Qt::darkGreen;
        attr.designationInfo = "";
        attr.type = "";
        attr.typeColor = Qt::darkGreen;
        attr.typeInfo = "";
    }
}

void SignalVisualizer::initControlSignal(SignalAttributes& attr, const QString& name, const QString& info) {
    attr.designation = name;
    attr.designationColor = Qt::magenta;
    attr.designationInfo = info;
    attr.type = "Control Signals";
    attr.typeColor = Qt::magenta;
    attr.typeInfo = "Обеспечивают управление работой устройств, устанавливая их состояния,\nактивируя режимы функционирования или обеспечивая синхронизацию через тактирование";
}

void SignalVisualizer::initDataSignal(SignalAttributes& attr, const QString& name, const QString& info) {
    attr.designation = name;
    attr.designationColor = Qt::blue;
    attr.designationInfo = info;
    attr.type = "Data Signals";
    attr.typeColor = Qt::blue;
    attr.typeInfo = "Предназначены для передачи и приёма цифровых данных";
}

void SignalVisualizer::applyLineAppearance(const SignalAttributes& attr, NetConnections& net) {
    setDesignation(attr.designation, net);
    setDesignationLineColor(attr.designationColor, net);
    setDesignationInfo(attr.designationInfo, net);
    setType(attr.type, net);
    setTypeLineColor(attr.typeColor, net);
    setTypeInfo(attr.typeInfo, net);
}

void SignalVisualizer::updateConnectionsMap(Pin* startPin, Pin* endpin, QList<QGraphicsLineItem*>& lineItems) {
    QString startId = startPin->pinId();
    QString endId = endpin->pinId();
    bool startIsNode = startId.contains("Node", Qt::CaseInsensitive);
    bool endIsNode = endId.contains("Node", Qt::CaseInsensitive);
    QString mapKeyStart = startIsNode ? findMatchingKeyForNodeConnection(startId, m_netConnections): findMatchingKeyForPin(startId, m_netConnections);
    QString mapKeyEnd   = endIsNode ? findMatchingKeyForNodeConnection(endId, m_netConnections) : findMatchingKeyForPin(endId, m_netConnections);
    QString targetKey;

    if (!mapKeyStart.isEmpty()) targetKey = mapKeyStart;
    else if (!mapKeyEnd.isEmpty()) targetKey = mapKeyEnd;

    if (!targetKey.isEmpty()) {
        m_netConnections[targetKey].lineList.append(lineItems);
        m_netConnections[targetKey].pinList.append(startPin);
        m_netConnections[targetKey].pinList.append( endpin);
        if (!mapKeyStart.isEmpty() && !mapKeyEnd.isEmpty() && mapKeyStart != mapKeyEnd) {
            mergeDuplicateKeysNodes(m_netConnections);
        }
    } else {
        QList<Pin*> pins = { startPin,  endpin };
        QString newKey = (!startIsNode && endIsNode) ? startId : (startIsNode && !endIsNode) ? endId : startId;
        m_netConnections[newKey] = NetConnections(lineItems, pins);
    }
}

void SignalVisualizer::mergeDuplicateKeysNodes(QMap<QString, NetConnections>& m_netConnections) {
    QList<QString> keysToRemove;
    QList<QString> allKeys = m_netConnections.keys();
    QHash<QString, QSet<QString>> nodeGroups;

    for (const QString& key : allKeys) {
        if (!m_netConnections.contains(key)) continue;
        
        NetConnections& net = m_netConnections[key];
        for (Pin* pin : net.pinList) {
            QString pinId = pin->pinId();
            QRegularExpressionMatch match = QRegularExpression("^Node-(\\d+)-", QRegularExpression::CaseInsensitiveOption).match(pinId);
            if (match.hasMatch()) {
                QString nodeNum = match.captured(1);
                nodeGroups[nodeNum].insert(key);
            }
        }
    }

    for (const auto& group : nodeGroups) {
        if (group.size() < 2) continue;

        QString mainKey = *group.begin();
        if (!m_netConnections.contains(mainKey)) continue;

        NetConnections& mainNet = m_netConnections[mainKey];
        QSet<QGraphicsLineItem*> uniqueLines(mainNet.lineList.begin(), mainNet.lineList.end());
        QSet<Pin*> uniquePins(mainNet.pinList.begin(), mainNet.pinList.end());

        for (const QString& key : group) {
            if (key == mainKey || !m_netConnections.contains(key)) continue;
            
            NetConnections& net = m_netConnections[key];
            uniqueLines.unite(QSet<QGraphicsLineItem*>(net.lineList.begin(), net.lineList.end()));
            uniquePins.unite(QSet<Pin*>(net.pinList.begin(), net.pinList.end()));
            keysToRemove.append(key);
        }

        mainNet.lineList = QList<QGraphicsLineItem*>(uniqueLines.begin(), uniqueLines.end());
        mainNet.pinList = QList<Pin*>(uniquePins.begin(), uniquePins.end());
    }
    // Удаляем дубликаты
    for (const QString& key : keysToRemove) {
        m_netConnections.remove(key);
    }
}

QString SignalVisualizer::findMatchingKeyForNodeConnection(const QString& pinId, QMap<QString, NetConnections>& m_netConnections) {
    QRegularExpression regex("^Node-(\\d+)-\\d+$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(pinId);

    if (!match.hasMatch()) return QString();

    QString nodeGroup = match.captured(1);

    for (auto it = m_netConnections.begin(); it != m_netConnections.end(); ++it) {
        const QList<Pin*>& pinList = it.value().pinList;

        for (Pin* pin : pinList) {
            if (!pin) continue;

            QString otherPinId = pin->pinId();
            QRegularExpressionMatch otherMatch = regex.match(otherPinId);
            if (otherMatch.hasMatch() && otherMatch.captured(1) == nodeGroup) {
                return it.key();
            }
        }
    }

    return QString();
}

QString SignalVisualizer::findMatchingKeyForPin(const QString& pinId, QMap<QString, NetConnections>& m_netConnections){
    for (auto it = m_netConnections.begin(); it != m_netConnections.end(); ++it) {
        QList<Pin*> pinList = it.value().pinList;

        for (Pin* pin : pinList) {
            QString currentPinId = pin->pinId();
            if (pinId == currentPinId) {
                return it.key();
            }
        }
    }
    return QString();
}

void SignalVisualizer::assignVoltageGradientColors() {
    QRegularExpression voltageRegex(R"(^([+-]?\d+(?:\.\d+)?)(V|VBATT[+-])$)");

    QMap<QString, double> typeToVoltage;
    QMap<QString, QList<QString>> typeToKeys;

    for (auto it = m_netConnections.begin(); it != m_netConnections.end(); ++it) {
        const QString& key = it.key();
        NetConnections& conn = it.value();

        if (conn.type != "Power")
            continue;

        QRegularExpressionMatch match = voltageRegex.match(conn.designation);
        if (!match.hasMatch())
            continue;

        double voltage;
        QString voltageStr = match.captured(1);
        if (voltageStr.startsWith("+") || voltageStr.startsWith("-")) {
            voltage = voltageStr.toDouble();
        } else {
            voltage = match.captured(0).replace(match.captured(2), "").toDouble();
        }

        if (!typeToVoltage.contains(conn.designation)) {
            typeToVoltage[conn.designation] = voltage;
        }
        typeToKeys[conn.designation].append(key);
    }

    if (typeToVoltage.isEmpty())
        return;

    QList<double> voltages = typeToVoltage.values();
    double minV = *std::min_element(voltages.begin(), voltages.end());
    double maxV = *std::max_element(voltages.begin(), voltages.end());

    for (auto it = typeToVoltage.begin(); it != typeToVoltage.end(); ++it) {
        const QString& type = it.key();
        double voltage = it.value();

        double t = (maxV == minV) ? 1.0 : (voltage - minV) / (maxV - minV);
        int red = static_cast<int>(155 + t * (255 - 155));
        QColor color(red, 0, 0);

        for (const QString& key : typeToKeys[type]) {
            m_netConnections[key].designationColor = color;
            applyColorToLineGroup(color, m_netConnections[key].lineList);
        }
    }
}

QString SignalVisualizer::formatVoltage(double value) {
    if (std::floor(value) == value) {
        return QString::asprintf("%+.0f", value);
    } else {
        return QString::asprintf("%+.1f", value);
    }
}

bool SignalVisualizer::hasPin(const QString& pinId, const QString& name) {
    return pinId.contains(name, Qt::CaseInsensitive);
}

bool SignalVisualizer::matchRegex(const QString& pinId, const QString& pattern) {
    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption)
        .match(pinId).hasMatch();
}

void SignalVisualizer::setDesignation(const QString& designation, NetConnections& net) {
    net.designation = designation;
}

void SignalVisualizer::setType(const QString& type, NetConnections& net) {
    net.type = type;
}

void SignalVisualizer::setDesignationLineColor(const QColor& color, NetConnections& net) {
    net.designationColor = color;
}

void SignalVisualizer::setTypeLineColor(const QColor& color, NetConnections& net) {
    net.typeColor = color;
}

void SignalVisualizer::setDesignationInfo(const QString& info, NetConnections& net) {
    net.designationInfo = info;
}

void SignalVisualizer::setTypeInfo(const QString& info, NetConnections& net) {
    net.typeInfo = info;
}

void SignalVisualizer::applyColorToLineGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup) {
    for (QGraphicsLineItem* line : lineGroup) {
        QPen currentPen = line->pen();
        currentPen.setColor(color);
        line->setPen(currentPen);
    }
}

void SignalVisualizer::applyThicknessToLineGroup(int thickness, const QList<QGraphicsLineItem*>& lineGroup) {
    for (QGraphicsLineItem* line : lineGroup) {
        if (line) {
            QPen pen = line->pen();
            pen.setWidth(thickness);
            line->setPen(pen);
        }
    }
}

void SignalVisualizer::updateNetColors(bool showCategories) {
    for (auto& connection : m_netConnections) {
        if (showCategories){
            applyColorToLineGroup(connection.typeColor, connection.lineList);
        } else {
            applyColorToLineGroup(connection.designationColor, connection.lineList);
        }
    }
}

void SignalVisualizer::setDesignationByGroup(const QString& designation, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            connection.designation = designation;
        }
    }
}

void SignalVisualizer::setDesignationInfoByGroup(const QString& info, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());
    QString targetDesignation, targetType;

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            targetDesignation = connection.designation;
            targetType = connection.type;
            break;
        }
    }

    for (auto& connection : m_netConnections) {
        if (connection.designation == targetDesignation && connection.type == targetType) {
            connection.designationInfo = info;
        }
    }
}

void SignalVisualizer::setTypeByGroup(const QString& type, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            connection.type = type;
        }
    }
}

void SignalVisualizer::setTypeInfoByGroup(const QString& info, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());
    QString  targetType;

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            targetType = connection.type;
            break;
        }
    }

    for (auto& connection : m_netConnections) {
        if (connection.type == targetType) {
            connection.typeInfo = info;
        }
    }
}

void SignalVisualizer::setDesignationLineColorByGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());
    QString targetDesignation, targetType;

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            targetDesignation = connection.designation;
            targetType = connection.type;
            break;
        }
    }

    for (auto& connection : m_netConnections) {
        if (connection.designation == targetDesignation && connection.type == targetType) {
            connection.designationColor = color;
        }
    }
}

void SignalVisualizer::setTypeLineColorByGroup(QColor color, QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());
        QString targetType;

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            targetType = connection.type;
            break;
        }
    }

    for (auto& connection : m_netConnections) {
        if (connection.type == targetType) {
            connection.typeColor = color;
        }
    }
}

QList<QString> SignalVisualizer::getExtractedDesignations() const {
    QList<QString> result;

    for (const auto &net : m_netConnections) {
        if (!net.designation.isEmpty() && !result.contains(net.designation)) {
            result.append(net.designation);
        }
    }
    return result;
}

QList<QString> SignalVisualizer::getExtractedCategories() const {
    QList<QString> result;

    for (const auto &net : m_netConnections) {
        if (!net.type.isEmpty() && !result.contains(net.type)) {
            result.append(net.type);
        }
    }

    return result;
}

QColor SignalVisualizer::getLineColorByGroup(const QList<QGraphicsLineItem*>& lineGroup, bool isShowingTypes) const {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());
    for (const auto& connection :  m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());

        if (inputSet == connectionSet) {
            SignalVisualizerWidget* widget = qobject_cast<SignalVisualizerWidget*>(parent());
            if (widget) {
                if(isShowingTypes) {
                    return connection.typeColor;
                } else {
                    return connection.designationColor;
                }
            }
        }
    }
    return QColor(255, 105, 180); // Если линия не найдена, возвращаем розовый цвет
}

QColor SignalVisualizer::getDesignationColorByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QSet<QGraphicsLineItem*> in(lineGroup.begin(), lineGroup.end());
    for (auto &net :  m_netConnections) {
        if (QSet<QGraphicsLineItem*>(net.lineList.begin(), net.lineList.end()) == in)
            return net.designationColor;
    }
    return QColor();
}

QColor SignalVisualizer::getTypeColorByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QSet<QGraphicsLineItem*> in(lineGroup.begin(), lineGroup.end());
    for (auto &net :  m_netConnections) {
        if (QSet<QGraphicsLineItem*>(net.lineList.begin(), net.lineList.end()) == in)
            return net.typeColor;
    }
    return QColor();
}

QList<QColor> SignalVisualizer::getColorsByDesignation(const QString &designation) const {
    QSet<QColor> colorSet;
    for (const auto &connection :  m_netConnections) {
        if (connection.designation == designation && connection.designationColor.isValid()) {
            colorSet.insert(connection.designationColor);
        }
    }
    return colorSet.values();
}

QList<QColor> SignalVisualizer::getColorsByType(const QString &type) const {
    QSet<QColor> colorSet;
    for (const auto &connection :  m_netConnections) {
        if (connection.type == type && connection.typeColor.isValid()) {
            colorSet.insert(connection.typeColor);
        }
    }
    return colorSet.values();
}

QString SignalVisualizer::getDesignationByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QString result;
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());

    for (const auto& connection :  m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            return connection.designation;
        }
    }
    return result;
}

QString SignalVisualizer::getTypeByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QString result;
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());

    for (const auto& connection :  m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            return connection.type;
        }
    }
    return result;
}

QList<int> SignalVisualizer::getThicknessesByDesignation(const QString &designation) const {
    QSet<int> thicknessSet;
    for (const auto &connection :  m_netConnections) {
        if (connection.designation == designation && !connection.lineList.isEmpty()) {
            const auto *line = connection.lineList.first();
            if (line) {
                thicknessSet.insert(static_cast<int>(std::round(line->pen().widthF())));
            }
        }
    }
    return thicknessSet.values();
}

QString SignalVisualizer::getTypeInfoByLine(QGraphicsLineItem* lineItem) const {
    for (const auto& connection :  m_netConnections) {
        if (connection.lineList.contains(lineItem)) {
            return connection.designationInfo;
        }
    }
    return QString();
}

QString SignalVisualizer::getDesignationInfoByLine(QGraphicsLineItem* lineItem) const {
    for (const auto& connection :  m_netConnections) {
        if (connection.lineList.contains(lineItem)) {
            return connection.typeInfo;
        }
    }
    return QString();
}

QList<QGraphicsLineItem*> SignalVisualizer::getGroupByLine(QGraphicsLineItem* lineItem) const {
    QList<QGraphicsLineItem*> result;
    for (const auto& connection :  m_netConnections) {
        if (connection.lineList.contains(lineItem)) {
            result = connection.lineList;
        }
    }
    return result;
}

QString SignalVisualizer::getDesignationInfoByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QSet<QGraphicsLineItem*> in(lineGroup.begin(), lineGroup.end());
    for (auto &net : m_netConnections) {
        if (QSet<QGraphicsLineItem*>(net.lineList.begin(), net.lineList.end()) == in)
            return net.designationInfo;
    }
    return QString();
}

QString SignalVisualizer::getTypeInfoByGroup(const QList<QGraphicsLineItem*>& lineGroup) const {
    QSet<QGraphicsLineItem*> in(lineGroup.begin(), lineGroup.end());
    for (auto &net : m_netConnections) {
        if (QSet<QGraphicsLineItem*>(net.lineList.begin(), net.lineList.end()) == in)
            return net.typeInfo;
    }
    return QString();
}

QString SignalVisualizer::getPositionalDesignation(const QString &type) {
    QString group = m_typeToGroup.value(type, type);

    auto it = m_posDesignation.find(group);
    if (it == m_posDesignation.end()) {
        return QString();
    }

    QString prefix = it->first;
    int index = it->second++;

    return prefix + QString::number(index);
}

void SignalVisualizer::removeDesignationForConnections(QString designation) {
    for (auto& connection : m_netConnections) {
        if (connection.designation == designation) {
            setDesignation("", connection);
            setDesignationLineColor(Qt::darkGreen, connection);
            setType("", connection);
            setTypeLineColor(Qt::darkGreen, connection);
            applyThicknessToLineGroup(3, connection.lineList);

            SignalVisualizerWidget* widget = qobject_cast<SignalVisualizerWidget*>(parent());
            QSet<QGraphicsLineItem*> selectedLinesSet;
            if (widget) {
                selectedLinesSet = QSet<QGraphicsLineItem*>(
                    widget->getView()->getSelectedLineGroup().cbegin(),
                    widget->getView()->getSelectedLineGroup().cend()
                );
            }
            QSet<QGraphicsLineItem*> connectionLinesSet = QSet<QGraphicsLineItem*>(
                connection.lineList.cbegin(),
                connection.lineList.cend()
            );
            if (selectedLinesSet != connectionLinesSet) {
                applyColorToLineGroup(Qt::darkGreen, connection.lineList); 
            }
        }
    }
}

void SignalVisualizer::removeTypeForConnections(const QString& type) {
    for (auto& connection : m_netConnections) {
        if (connection.type == type) {
            setDesignation("", connection);
            setType("", connection);
            setDesignationLineColor(Qt::darkGreen, connection);
            setTypeLineColor(Qt::darkGreen, connection);
            applyThicknessToLineGroup(3, connection.lineList);

            SignalVisualizerWidget* widget = qobject_cast<SignalVisualizerWidget*>(parent());
            QSet<QGraphicsLineItem*> selectedLinesSet;
            if (widget) {
                selectedLinesSet = QSet<QGraphicsLineItem*>(
                    widget->getView()->getSelectedLineGroup().cbegin(),
                    widget->getView()->getSelectedLineGroup().cend()
                );
            }
            QSet<QGraphicsLineItem*> connectionLinesSet = QSet<QGraphicsLineItem*>(
                connection.lineList.cbegin(),
                connection.lineList.cend()
            );
            if (selectedLinesSet != connectionLinesSet) {
                applyColorToLineGroup(Qt::darkGreen, connection.lineList);
            }
        }
    }
}

void SignalVisualizer::resetConnectionsByGroup(const QList<QGraphicsLineItem*>& lineGroup) {
    QSet<QGraphicsLineItem*> inputSet = QSet<QGraphicsLineItem*>(lineGroup.begin(), lineGroup.end());

    for (auto& connection : m_netConnections) {
        QSet<QGraphicsLineItem*> connectionSet = QSet<QGraphicsLineItem*>(connection.lineList.begin(), connection.lineList.end());
        if (inputSet == connectionSet) {
            setDesignation("", connection);
            setDesignationInfo("", connection);
            setType("", connection);
            setTypeInfo("", connection);
            setDesignationLineColor(Qt::darkGreen, connection);
            setTypeLineColor(Qt::darkGreen, connection);
            applyThicknessToLineGroup(3, connection.lineList);
        }
    }
}

QString SignalVisualizer::toString() {
    QString result = "<colorSchemeConfig>";

    for (auto it = m_netConnections.begin(); it != m_netConnections.end(); ++it) {
        QString netName = it.key();
        const NetConnections& connection = it.value();

        QString lineWidth = "1";
        if (!connection.lineList.isEmpty()) {
            lineWidth = QString::number(connection.lineList.first()->pen().widthF());
        }

        result += "\n<net name=\"" + netName +
                  "\" designation=\"" + connection.designation +
                  "\" designationInfo=\"" + connection.designationInfo.toHtmlEscaped().replace("\n", "&#10;") +
                  "\" designationLineColor=\"" + connection.designationColor.name() +
                  "\" type=\"" + connection.type +
                  "\" typeInfo=\"" + connection.typeInfo.toHtmlEscaped().replace("\n", "&#10;") +
                  "\" typeLineColor=\"" + connection.typeColor.name() +
                  "\" lineWidth=\"" + lineWidth + "\">\n";

        for (Pin* pin : connection.pinList) {
            if (pin)
                result += "  <pin id=\"" + pin->pinId() + "\"/>\n";
        }

        result += "</net>\n";
    }
    result += "</colorSchemeConfig>\n";
    return result;
}

void SignalVisualizer::loadFromString(const QString &xmlString) {
    QXmlStreamReader xml(xmlString);
    bool hasError = false;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement && xml.name() == "net") {
            QXmlStreamAttributes attrs = xml.attributes();
            QString netName = attrs.value("name").toString();
            QString designation = attrs.value("designation").toString();
            
            // Добавляем чтение информации с заменой HTML-сущностей
            QString designationInfo = attrs.value("designationInfo").toString()
                .replace("&#10;", "\n")
                .replace("&amp;", "&");
            
            QString type = attrs.value("type").toString();
            
            QString typeInfo = attrs.value("typeInfo").toString()
                .replace("&#10;", "\n")
                .replace("&amp;", "&");

            QColor designationColor(attrs.value("designationLineColor").toString());
            QColor typeColor(attrs.value("typeLineColor").toString());
            int thickness = attrs.value("lineWidth").toInt();

            QSet<QString> pinIds;
            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "net")) {
                xml.readNext();
                if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == "pin") {
                    pinIds.insert(xml.attributes().value("id").toString());
                }
            }

            // Обновляем данные соединения
            for (auto &connection : m_netConnections) {
                QSet<QString> currentPins;
                for (Pin *pin : connection.pinList) {
                    currentPins.insert(pin->pinId());
                }

                if (currentPins == pinIds) {
                    connection.designation = designation;
                    connection.designationInfo = designationInfo;
                    connection.type = type;
                    connection.typeInfo = typeInfo;
                    connection.designationColor = designationColor;
                    connection.typeColor = typeColor;

                    SignalVisualizerWidget* parentWidget = qobject_cast<SignalVisualizerWidget*>(parent());
                    if (parentWidget && parentWidget->getView()->isShowingTypes()) {
                        applyColorToLineGroup(typeColor, connection.lineList);
                    } else {
                        applyColorToLineGroup(designationColor, connection.lineList);
                    }
                    applyThicknessToLineGroup(thickness, connection.lineList);
                    break;
                }
            }
        }
    }

    if (xml.hasError()) {
        SignalVisualizerWidget* parentWidget = qobject_cast<SignalVisualizerWidget*>(parent());
        if (parentWidget) {
            QMessageBox::warning(parentWidget, tr("Ошибка XML"), 
                tr("Ошибка в строке %1: %2")
                    .arg(xml.lineNumber())
                    .arg(xml.errorString()));
        }
        hasError = true;
    }

    if (!hasError) {
        emit comboUpdated();
        emit legendUpdated();
        emit sceneUpdated();
    }
}
