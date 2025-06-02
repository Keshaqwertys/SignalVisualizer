#ifndef SIGNALVISUALIZERWIDGET_H
#define SIGNALVISUALIZERWIDGET_H

#include <QWidget>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsScene>
#include <QFileDialog>
#include <QMessageBox>
#include <QDataStream>
#include <QFile>
#include <QXmlStreamReader>
#include "signalvisualizer.h"
#include "signalvisualizerview.h"
#include "circuit.h"

class SignalVisualizer;

class SignalVisualizerWidget : public QWidget
{
    Q_OBJECT
    friend class SignalVisualizerView;
    friend class SignalVisualizer;

public:
    explicit SignalVisualizerWidget(QWidget *parent = nullptr);
    ~SignalVisualizerWidget();

public slots:
    void updateScene();

private:
    void setupUI();
    void applyStyles();
    void closeVisualizer();

    void saveConfig();
    void saveConfig(QString &file);
    void saveAsConfig();
    void loadConfig(const QString &fileName);
    void loadConfig();
    void showHelp();

    Circuit* getCircuit() const {return m_circuitInstance;};
    SignalVisualizer* getModel() const { return m_visualizerModel; }
    SignalVisualizerView* getView() const { return m_graphicsView; }

    Circuit* m_circuitInstance;
    SignalVisualizerView* m_graphicsView;
    SignalVisualizer* m_visualizerModel;

    QVBoxLayout* m_layout;

    QGraphicsScene* m_scene;
    QLabel* m_label;
    QPushButton* m_closeButton;

    QMenu* m_fileMenu;
    QMenuBar* m_menuBar;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;

    QString m_currentFileName;
    QString m_lastFileName;
};

#endif // SIGNALVISUALIZERWIDGET_H
