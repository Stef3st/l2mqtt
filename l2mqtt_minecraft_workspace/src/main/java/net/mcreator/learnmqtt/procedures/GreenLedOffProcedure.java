package net.mcreator.learnmqtt.procedures;

import java.util.Map;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class GreenLedOffProcedure {
	public static void executeProcedure(Map<String, Object> dependencies) {
		MqttAsyncClient mqttClient = MqttHandler.createMQTT();

		if(mqttClient == null) {
			System.out.println("No Mqtt client");
			return;
		}
			
		try {
		MqttMessage mqttMessage = new MqttMessage("OFF".getBytes());
		mqttClient.publish("l2mqtt/led/green", mqttMessage);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
