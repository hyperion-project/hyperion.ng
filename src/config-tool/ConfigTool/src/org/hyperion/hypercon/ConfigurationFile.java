package org.hyperion.hypercon;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.util.Properties;
import java.util.Vector;

public class ConfigurationFile {
	private final Properties mProps = new Properties();

	public void load(String pFilename) {
		mProps.clear();
//		try (InputStream in = new InflaterInputStream(new FileInputStream(pFilename))){
//		try (InputStream in = new GZIPInputStream(new FileInputStream(pFilename))){
		try (InputStream in = new FileInputStream(pFilename)) {
			mProps.load(in);
		} catch (Throwable t) {
			// TODO Auto-generated catch block
			t.printStackTrace();
		}
	}

	public void save(String pFilename) {
//		try (OutputStream out = new DeflaterOutputStream(new FileOutputStream(pFilename))) {
//		try (OutputStream out = new GZIPOutputStream(new FileOutputStream(pFilename))) {
		try (OutputStream out = (new FileOutputStream(pFilename))) {
			mProps.store(out, "Pesistent settings file for HyperCon");
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void store(Object pObj) {
		store(pObj, pObj.getClass().getSimpleName(), "");
	}
	public void store(Object pObj, String preamble, String postamble) {
		String className = pObj.getClass().getSimpleName();
		// Retrieve the member variables
		Field[] fields = pObj.getClass().getDeclaredFields();
		// Iterate each variable
		for (Field field : fields) {
			if (!Modifier.isPublic(field.getModifiers())) {
				System.out.println("Unable to synchronise non-public field(" + field.getName() + ") in configuration structure(" + className + ")");
				continue;
			}
			
			String key = preamble + "." + field.getName() + postamble;
			try {
				Object value = field.get(pObj);
				
				if (value.getClass().isEnum()) {
					mProps.setProperty(key, ((Enum<?>)value).name());
				} else if (value.getClass().isAssignableFrom(Vector.class)) {
					@SuppressWarnings("unchecked")
					Vector<Object> v = (Vector<Object>) value; 
					for (int i=0; i<v.size(); ++i) {
						store(v.get(i), key + "[" + i + "]", "");
					}
				} else {
					mProps.setProperty(key, value.toString());
				}
			} catch (Throwable t) {} 
		}
	}
	
	public void restore(Object pObj) {
		restore(pObj, mProps);
	}
	
	public void restore(Object pObj, Properties pProps) {
		String className = pObj.getClass().getSimpleName();
		restore(pObj, pProps, className + ".");
	}
	
	@SuppressWarnings("unchecked")
	public void restore(Object pObj, Properties pProps, String pPreamble) {
		// Retrieve the member variables
		Field[] fields = pObj.getClass().getDeclaredFields();
		// Iterate each variable
		for (Field field : fields) {
			if (field.getType().isAssignableFrom(Vector.class)) {
				// Obtain the Vector
				Vector<Object> vector;
				try {
					vector = (Vector<Object>)field.get(pObj);
				} catch (Throwable t) {
					t.printStackTrace();
					break;
				}
				// Clear existing elements from the vector
				vector.clear();

				// Iterate through the properties to find the indices of the vector
				int i=0;
				while (true) {
					String curIndexKey = pPreamble + field.getName() + "[" + i + "]";
					Properties elemProps = new Properties();
					// Find all the elements for the current vector index
					for (Object keyObj : pProps.keySet()) {
						String keyStr = (String)keyObj;
						if (keyStr.startsWith(curIndexKey)) {
							// Remove the name and dot
							elemProps.put(keyStr.substring(curIndexKey.length()+1), pProps.get(keyStr));
						}
					}
					if (elemProps.isEmpty()) {
						// Found no more elements for the vector
						break;
					}
					
					// Construct new instance of vectors generic type
					ParameterizedType vectorElementType = (ParameterizedType) field.getGenericType();
					Class<?> vectorElementClass = (Class<?>) vectorElementType.getActualTypeArguments()[0];
					// Find the constructor with no arguments and create a new instance
					Object newElement = null;
					try {
						newElement = vectorElementClass.getConstructor().newInstance();
					} catch (Throwable t) {
						System.err.println("Failed to find empty default constructor for " + vectorElementClass.getName());
						break;
					}
					if (newElement == null) {
						System.err.println("Failed to construct instance for " + vectorElementClass.getName());
						break;
					}
					
					// Restore the instance members from the collected properties
					restore(newElement, elemProps, "");
					
					// Add the instance to the vector
					vector.addElement(newElement);
					
					++i;
				}				
				
				continue;
			}
			
			String key = pPreamble + field.getName();
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
		return mProps.toString();
	}
}
