package net.mcreator.learnmqtt.procedures;

import java.util.Map;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClientPersistence;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MqttHandler {
	private static MqttAsyncClient mqttClient;
	private static final String BROKER    = "tcp://test.mosquitto.org:1883";
	private static final String CLIENT_ID = "MINECRAFT-CLIENT-ID"

	public static MqttAsyncClient createMQTT() {
		if(mqttClient != null)
			return mqttClient;
		try {
			mqttClient = new MqttAsyncClient(BROKER, CLIENT_ID, (MqttClientPersistence) null);
			MqttConnectOptions mqttConnectOptions = generateMQTTConnectOptions();

			IMqttToken tok = mqttClient.connect(mqttConnectOptions);
			tok.waitForCompletion();
			return mqttClient;
		} catch (Exception e) {
			System.out.println("Client:MINECRAFT-CLIENT-ID :  " + e.getMessage());
			return null;
		}
	}

	private static MqttConnectOptions generateMQTTConnectOptions() throws Exception {
		MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
		mqttConnectOptions.setCleanSession(true);
		mqttConnectOptions.setConnectionTimeout(30);
		mqttConnectOptions.setKeepAliveInterval(60);
		mqttConnectOptions.setMaxInflight(10000);
		//If a username and password is required, uncomment the lines below.
		/*
		mqttConnectOptions.setUserName("username");
		mqttConnectOptions.setPassword("password".toCharArray());
		*/
		return mqttConnectOptions;
	}
	
	//Can be used for testing purposes
	/*	
	public void connectionLost(Throwable cause) {
        System.out.println("Connection lost...");
	}
	
	public void messageArrived(String topic, MqttMessage message) {
		try {
			String msg = new String(message.getPayload());
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	*/
}
