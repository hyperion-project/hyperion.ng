#pragma once

// QT include
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

class QJsonUtils
{
public:

	static bool modify(QJsonValue& value, QStringList path, const QJsonValue& newValue = QJsonValue::Null, QString propertyName = "")
	{
		QJsonObject result;
		bool propertyUpdated {false};
		if (!path.isEmpty())
		{

			if (path.first() == "[root]")
			{
				path.removeFirst();
			}
			// Clean up leading periods in path elements
			for (auto& current : path) {
				if (current.startsWith("."))
				{
					current.remove(0, 1);
				}
			}

			if (! (value.toObject().isEmpty() && value.toArray().isEmpty()) )
			{
				modifyValue(value, result, path, newValue, propertyName, propertyUpdated);
			}
			else if (newValue != QJsonValue::Null && !propertyName.isEmpty())
			{
				result[propertyName] = newValue;
				propertyUpdated = true;
			}
		}
		value = result;
		return propertyUpdated;
	}

	static QJsonValue create(QJsonValue schema, bool ignoreRequired = false)
	{
		return createValue(schema, ignoreRequired);
	}

	static QString getDefaultValue(const QJsonValue & value)
	{
		QString ret;
		switch (value.type())
		{
			case QJsonValue::Array:
			{
				for (const QJsonValueRef v : value.toArray())
				{
					ret = getDefaultValue(v);
					if (!ret.isEmpty())
						break;
				}
				break;
			}
			case QJsonValue::Object:
			{
				ret = getDefaultValue(value.toObject().value("default"));
			}
				break;
			case QJsonValue::Bool:
				return value.toBool() ? "True" : "False";
			case QJsonValue::Double:
				return QString::number(value.toDouble());
			case QJsonValue::String:
				return value.toString();
			case QJsonValue::Null:
			case QJsonValue::Undefined:
				break;
		}
		return ret;
	}

private:

	static QJsonValue createValue(QJsonValue schema, bool ignoreRequired)
	{
		QJsonObject result;
		QJsonObject obj = schema.toObject();

		if (obj.find("type") != obj.end() && obj.find("type").value().isString())
		{
			QJsonValue ret = QJsonValue::Null;

			if (obj.find("type").value().toString() == "object" && ( obj.find("required").value().toBool() || ignoreRequired ) )
				ret = createValue(obj["properties"], ignoreRequired);
			else if (obj.find("type").value().toString() == "array" && ( obj.find("required").value().toBool() || ignoreRequired ) )
			{
				QJsonArray array;

				if (obj.find("default") != obj.end())
					ret = obj.find("default").value();
				else
				{
					ret = createValue(obj["items"], ignoreRequired);

					if (!ret.toObject().isEmpty())
						array.append(ret);
					ret = array;
				}
			}
			else if ( obj.find("required").value().toBool() || ignoreRequired )
				if (obj.find("default") != obj.end())
					ret = obj.find("default").value();

			return ret;
		}
		else
		{
			for (QJsonObject::const_iterator i = obj.begin(); i != obj.end(); ++i)
			{
				QString attribute = i.key();
				const QJsonValue & attributeValue = *i;
				QJsonValue subValue = obj[attribute];

				if (attributeValue.toObject().find("type") != attributeValue.toObject().end())
				{
					if (attributeValue.toObject().find("type").value().toString() == "object" && ( attributeValue.toObject().find("required").value().toBool() || ignoreRequired ) )
					{
						if (obj.contains("properties"))
							result[attribute] = createValue(obj["properties"], ignoreRequired);
						else
							result[attribute] = createValue(subValue, ignoreRequired);
					}
					else if (attributeValue.toObject().find("type").value().toString() == "array" && ( attributeValue.toObject().find("required").value().toBool() || ignoreRequired ) )
					{
						QJsonArray array;

						if (attributeValue.toObject().find("default") != attributeValue.toObject().end())
							result[attribute] = attributeValue.toObject().find("default").value();
						else
						{
							QJsonValue retEmpty;
							retEmpty = createValue(attributeValue.toObject()["items"], ignoreRequired);

							if (!retEmpty.toObject().isEmpty())
								array.append(retEmpty);
							result[attribute] = array;
						}
					}
					else if ( attributeValue.toObject().find("required").value().toBool() || ignoreRequired )
					{
						if (attributeValue.toObject().find("default") != attributeValue.toObject().end())
							result[attribute] = attributeValue.toObject().find("default").value();
						else
							result[attribute] = QJsonValue::Null;
					}
				}
			}
		}

		return result;
	}

	static void modifyValue(QJsonValue source, QJsonValue target, QStringList path, const QJsonValue& newValue, QString& property, bool& propertyUpdated)
	{
		// Ensure the path is not empty
		if (path.isEmpty()) {
			propertyUpdated = false;
			return;
		}

		QString current = path.takeFirst();

		// Handle object case
		if (source.isObject()) {
			QJsonObject obj = source.toObject();

			if (!obj.contains(current)) {
				// Create missing key if necessary
				if (path.isEmpty()) {
					obj[current] = newValue;
					propertyUpdated = true;
				} else {
					QJsonValue emptyObj((QJsonObject())); // create an empty object
					modifyValue(emptyObj, obj[current], path, newValue, property, propertyUpdated);
				}
			} else {
				QJsonValue existingValue = obj[current];
				modifyValue(existingValue, obj[current], path, newValue, property, propertyUpdated);
			}

			target = obj; // Reassign modified object to target
		}
		// Handle array case
		else if (source.isArray()) {
			QJsonArray arr = source.toArray();
			bool isIndex;
			int index = current.toInt(&isIndex);

			if (isIndex && index >= 0 && index < arr.size()) {
				// If the path is empty, modify the element at the index
				if (path.isEmpty()) {
					arr[index] = newValue;
					propertyUpdated = true;
				} else {
					QJsonValue arrayElement = arr[index];
					modifyValue(arrayElement, arr[index], path, newValue, property, propertyUpdated);
				}
			}
			else {
				// Expand array if index is out of bounds
				while (arr.size() <= index)
					arr.append(QJsonValue());

				QJsonValue arrayElement = arr[index];
				modifyValue(arrayElement, arr[index], path, newValue, property, propertyUpdated);
			}

			target = arr; // Reassign modified array to target
		}
		// Handle unsupported cases (e.g., primitive values)
		else {
			if (path.isEmpty()) {
				QJsonObject obj;
				obj[current] = newValue;
				target = obj;
				propertyUpdated = true;
			}
		}
	}

};
