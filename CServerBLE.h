#ifndef CSERVERBLE_H
#define CSERVERBLE_H

#include <QtBluetooth/qlowenergyadvertisingdata.h>
#include <QtBluetooth/qlowenergyadvertisingparameters.h>
#include <QtBluetooth/qlowenergycharacteristic.h>
#include <QtBluetooth/qlowenergycharacteristicdata.h>
#include <QtBluetooth/qlowenergydescriptordata.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergyservice.h>
#include <QtBluetooth/qlowenergyservicedata.h>
#include <QtEndian>
#include <QFile>
#include <QTimer>

class CServerBLE : public QObject
{
    Q_OBJECT
public:
    CServerBLE();
    ~CServerBLE();

public:
    void initServer();

protected slots:
    void controllerStateChanged(QLowEnergyController::ControllerState state);
    void characteristicChanged(QLowEnergyCharacteristic c, QByteArray data);
    void updateTemperature();

protected:
    QLowEnergyCharacteristicData createCharacteristic(QBluetoothUuid uuid,  QLowEnergyCharacteristic::PropertyTypes type);
    void setValue(QBluetoothUuid uuid, quint32 value);

private:
    QLowEnergyAdvertisingData   m_advertisingData;
    QLowEnergyService           *m_serviceRelay;
    QLowEnergyController        *m_bleController;
    QFile                       *m_relay1File;
    QFile                       *m_relay2File;
    QTimer                      *m_timerTemperature;
};

#endif // CSERVERBLE_H
