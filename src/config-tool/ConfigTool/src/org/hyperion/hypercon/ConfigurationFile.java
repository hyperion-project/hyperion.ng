package org.hyperion.hypercon;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Properties;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

public class ConfigurationFile {
	private final Properties pProps = new Properties();

	public void load(String pFilename) {
		pProps.clear();
//		try (InputStream in = new InflaterInputStream(new FileInputStream(pFilename))){
		try (InputStream in = new GZIPInputStream(new FileInputStream(pFilename))){
//		try (InputStream in = new FileInputStream(pFilename)) {
			pProps.load(in);
		} catch (Throwable t) {
			// TODO Auto-generated catch block
			t.printStackTrace();
		}
	}

	public void save(String pFilename) {
//		try (OutputStream out = new DeflaterOutputStream(new FileOutputStream(pFilename))) {
		try (OutputStream out = new GZIPOutputStream(new FileOutputStream(pFilename))) {
//		try (OutputStream out = (new FileOutputStream(pFilename))) {
			pProps.store(out, "Pesistent settings file for HyperCon");
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void store(Object pObj) {
		String className = pObj.getClass().getSimpleName();
		// Retrieve the member variables
		Field[] fields = pObj.getClass().getDeclaredFields();
		// Iterate each variable
		for (Field field : fields) {
			if (!Modifier.isPublic(field.getModifiers())) {
				System.out.println("Unable to synchronise non-public field(" + field.getName() + ") in configuration structure(" + className + ")");
				continue;
			}
			
			String key = className + "." + field.getName();
			try {
				Object value = field.get(pObj);
				
				if (value.getClass().isEnum()) {
					pProps.setProperty(key, ((Enum<?>)value).name());
				} else {
					pProps.setProperty(key, value.toString());
				}
			} catch (Throwable t) {} 
		}
	}
	
	public void restore(Object pObj) {
		String className = pObj.getClass().getSimpleName();
		
		// Retrieve the member variables
		Field[] fields = pObj.getClass().getDeclaredFields();
		// Iterate each variable
		for (Field field : fields) {
			String key = className + "." + field.getName();
			String value = pProps.getProperty(key);
			if (value == null) {
				System.out.println("Persistent settings does not contain value for " + key);
				continue;
			}

			try {
				if (field.getType() == boolean.class) {
					field.set(pObj, Boolean.parseBoolean(value));
				} else if (field.getType() == int.class) {
					field.set(pObj, Integer.parseInt(value));
				} else if (field.getType() == double.class) {
					field.set(pObj, Double.parseDouble(value));
				} else if (field.getType().isEnum()) {
					Method valMet = field.getType().getMethod("valueOf", String.class);
					Object enumVal = valMet.invoke(null, value);
					field.set(pObj, enumVal);
				} else {
					field.set(pObj, value);
				}
			} catch (Throwable t) {
				System.out.println("Failed to parse value(" + value + ") for " + key);
			}
		}		
	}
	
	@Override
	public String toString() {
		return pProps.toString();
	}
}
