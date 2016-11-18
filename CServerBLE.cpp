#include "CServerBLE.h"

#define UUID_RELAY1         0x2aa4
#define UUID_RELAY2         0x2aa5
#define UUID_TEMPERATURE    0x2aa6

QByteArray readValueFromFile(QString filePath)
{
    QByteArray data;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        data = file.readAll();
        data.remove(data.length() - 1, 1);
        file.close();
    }

    return data;
}

CServerBLE::CServerBLE()
{
    m_relay1File = new QFile("/dev/gpio/out_RL1");
    m_relay2File = new QFile("/dev/gpio/out_RL2");
    m_timerTemperature = new QTimer;
    connect(m_timerTemperature, SIGNAL(timeout()), this, SLOT(updateTemperature()));
}

CServerBLE::~CServerBLE()
{
    delete m_relay1File;
    delete m_relay2File;
    delete m_timerTemperature;
}

void CServerBLE::initServer()
{
    QBluetoothUuid serviceRelayUuid((quint32)0x1820);
    QBluetoothUuid charRelay1Uuid((quint32)UUID_RELAY1);
    QBluetoothUuid charRelay2Uuid((quint32)UUID_RELAY2);
    QBluetoothUuid temperatureUuid((quint32)UUID_TEMPERATURE);

                                            //serveur
    m_advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);  //Mode d'accessibilité
    m_advertisingData.setLocalName("GLMF_200_BLE");                                    //Nom du serveur
    m_advertisingData.setServices(QList<QBluetoothUuid>() <<
                                serviceRelayUuid
                                );                         //Ajout des services prédéfinis

    //Création de la charactéristique
    QLowEnergyCharacteristicData charRelay1 = createCharacteristic(charRelay1Uuid,
                                                                   QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::Read);
    QLowEnergyCharacteristicData charRelay2 = createCharacteristic(charRelay2Uuid,
                                                                   QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::Read);
    QLowEnergyCharacteristicData charTemperature = createCharacteristic(temperatureUuid,
                                                                   QLowEnergyCharacteristic::Notify);

    //Couplage du service avec la caractéristique créée.
    QLowEnergyServiceData serviceRelayData;
    serviceRelayData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceRelayData.setUuid(serviceRelayUuid);
    serviceRelayData.addCharacteristic(charRelay1);
    serviceRelayData.addCharacteristic(charRelay2);
    serviceRelayData.addCharacteristic(charTemperature);

    //création du controlleur BLE
    m_bleController = QLowEnergyController::createPeripheral();
    connect(m_bleController,
            SIGNAL(stateChanged(QLowEnergyController::ControllerState)),
            this,
            SLOT(controllerStateChanged(QLowEnergyController::ControllerState)));

    //ajout du service
    m_serviceRelay = m_bleController->addService(serviceRelayData);

    //permettra de récupérer les données reçues
    connect(m_serviceRelay,
            SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
            this,
            SLOT(characteristicChanged(QLowEnergyCharacteristic,QByteArray)));

    //démarrage "advertising"
    m_bleController->startAdvertising(QLowEnergyAdvertisingParameters(), m_advertisingData, m_advertisingData);
}

void CServerBLE::controllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::UnconnectedState)
    {
        qDebug() << "Client Disconnected";
        m_bleController->startAdvertising(QLowEnergyAdvertisingParameters(), m_advertisingData, m_advertisingData);

        m_timerTemperature->stop();
    }

    if (state == QLowEnergyController::ConnectedState)
    {
        qDebug() << "Client Connected";

        //démarrage lecture température
        m_timerTemperature->start(500);
    }
}

void CServerBLE::characteristicChanged(QLowEnergyCharacteristic c, QByteArray data)
{
    switch(c.uuid().toUInt32())
    {
    case UUID_RELAY1:
        m_relay1File->open(QIODevice::ReadWrite);
        m_relay1File->write(data);
        m_relay1File->close();
        break;
    case UUID_RELAY2:
        m_relay2File->open(QIODevice::ReadWrite);
        m_relay2File->write(data);
        m_relay2File->close();
        break;
    }
}

void CServerBLE::updateTemperature()
{
    //récupération de la valeur de température
    QByteArray rawTemp = readValueFromFile("/sys/bus/iio/devices/iio\:device0/in_temp_raw");
    QByteArray scaleTemp = readValueFromFile("/sys/bus/iio/devices/iio\:device0/in_temp_scale");

    float temperature = rawTemp.toInt() * scaleTemp.toFloat();

    // récupération de la caratéristique du service
    QLowEnergyCharacteristic characteristic = m_serviceRelay->characteristic(QBluetoothUuid((quint32)UUID_TEMPERATURE));
    Q_ASSERT(characteristic.isValid());

    //mise à jour de la donnée
    m_serviceRelay->writeCharacteristic(characteristic, QByteArray::number(temperature)); // Potentially causes notification.
}

void CServerBLE::setValue(QBluetoothUuid uuid, quint32 value)
{
    QLowEnergyCharacteristic characteristic = m_serviceRelay->characteristic(uuid);
    Q_ASSERT(characteristic.isValid());
    m_serviceRelay->writeCharacteristic(characteristic, QByteArray::number(value)); // Potentially causes notification.
}

QLowEnergyCharacteristicData CServerBLE::createCharacteristic(QBluetoothUuid uuid, QLowEnergyCharacteristic::PropertyTypes type)
{
    QLowEnergyCharacteristicData charData;
    charData.setUuid(QBluetoothUuid(uuid)); //uuid définie de la charactéristique
    charData.setValue(QByteArray(2, 0));
    charData.setProperties(type); //précise le type de la propriété
    const QLowEnergyDescriptorData clientConfig(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    charData.addDescriptor(clientConfig);

    return charData;
}
