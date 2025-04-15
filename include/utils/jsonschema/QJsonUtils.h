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

	static void modify(QJsonValue& value, QStringList path, const QJsonValue& newValue = QJsonValue::Null, const QString& propertyName = "")
	{
		QJsonValue result;

		if (!path.isEmpty())
		{
			if (path.first() == "[root]")
			{
				path.removeFirst();
			}

			for (QString& pathItem : path)
			{
				if (pathItem.startsWith("."))
				{
					pathItem = pathItem.mid(1);
				}
			}

			if (!value.toObject().isEmpty() || !value.toArray().isEmpty())
			{
				modifyValue(value, result, path, newValue);
			}
			else if (newValue != QJsonValue::Null && !propertyName.isEmpty())
			{
				QJsonObject temp;
				temp[propertyName] = newValue;
				result = temp;
			}
		}

		value = result;
	}


	static QJsonValue create(const QJsonValue& schema, bool ignoreRequired = false)
	{
		return createValue(schema, ignoreRequired);
	}

private:

	static QJsonValue createValue(const QJsonValue& schema, bool ignoreRequired)
	{
		QJsonObject composedObject;
		QJsonObject const obj = schema.toObject();

		// Handle top-level schema with "type"
		auto typeIt = obj.constFind("type");
		if (typeIt != obj.constEnd() && typeIt->isString())
		{
			QString const typeStr = typeIt->toString();
			bool const isRequired = obj.value("required").toBool() || ignoreRequired;
			QJsonValue finalValue = QJsonValue::Null;

			if (typeStr == "object" && isRequired)
			{
				finalValue = createValue(obj.value("properties"), ignoreRequired);
			}
			else if (typeStr == "array" && isRequired)
			{
				if (obj.contains("default"))
				{
					finalValue = obj.value("default");
				}
				else
				{
					QJsonArray array;
					QJsonValue const itemValue = createValue(obj.value("items"), ignoreRequired);

					if (!itemValue.toObject().isEmpty())
					{
						array.append(itemValue);
					}

					finalValue = array;
				}
			}
			else if (isRequired)
			{
				if (obj.contains("default"))
				{
					finalValue = obj.value("default");
				}
			}

			return finalValue;
		}

		// If no top-level type, iterate through members
		for (QJsonObject::const_iterator it = obj.constBegin(); it != obj.constEnd(); ++it)
		{
			QString const attribute = it.key();
			const QJsonValue& attributeValue = it.value();
			QJsonObject const attrObj = attributeValue.toObject();

			auto attrTypeIt = attrObj.constFind("type");
			QString const attrType = (attrTypeIt != attrObj.constEnd()) ? attrTypeIt->toString() : QString();
			bool const attrIsRequired = attrObj.value("required").toBool() || ignoreRequired;

			if (!attrType.isEmpty())
			{
				if (attrType == "object" && attrIsRequired)
				{
					if (obj.contains("properties"))
					{
						composedObject.insert(attribute, createValue(obj.value("properties"), ignoreRequired));
					}
					else
					{
						composedObject.insert(attribute, createValue(attributeValue, ignoreRequired));
					}
				}
				else if (attrType == "array" && attrIsRequired)
				{
					if (attrObj.contains("default"))
					{
						composedObject.insert(attribute, attrObj.value("default"));
					}
					else
					{
						QJsonArray array;
						QJsonValue const itemsValue = createValue(attrObj.value("items"), ignoreRequired);

						if (!itemsValue.toObject().isEmpty())
						{
							array.append(itemsValue);
						}

						composedObject.insert(attribute, array);
					}
				}
				else if (attrIsRequired)
				{
					if (attrObj.contains("default"))
					{
						composedObject.insert(attribute, attrObj.value("default"));
					}
					else
					{
						composedObject.insert(attribute, QJsonValue::Null);
					}
				}
			}
		}

		return composedObject;
	}

	static void modifyValue(const QJsonValue& source, QJsonValue& target, QStringList path, const QJsonValue& newValue)
	{
		// Handle case where the source is an object
		if (source.isObject())
		{
			QJsonObject sourceObj = source.toObject();
			QJsonObject targetObj = target.isObject() ? target.toObject() : QJsonObject();

			bool foundKey = false;

			for (auto it = sourceObj.begin(); it != sourceObj.end(); ++it)
			{
				const QString& key = it.key();
				const QJsonValue& subValue = it.value();

				if (!path.isEmpty() && key == path.first())
				{
					path.takeFirst(); //Remove first item of path
					QJsonValue subTarget;
					modifyValue(subValue, subTarget, path, newValue);
					targetObj.insert(key, subTarget);
					foundKey = true;
				}
				else
				{
					targetObj.insert(key, subValue);
				}
			}

			// If key wasn't found and path is now size 1, create the key and insert newValue
			if (!path.isEmpty() && path.size() == 1 && !foundKey)
			{
				targetObj.insert(path.first(), newValue);
				path.clear();
			}

			target = targetObj;
		}

		// Handle case where the source is an array
		else if (source.isArray())
		{
			QJsonArray sourceArray = source.toArray();
			QJsonArray targetArray = target.isArray() ? target.toArray() : QJsonArray();

			int index = -1;
			if (!path.isEmpty() && path.first().startsWith("[") && path.first().endsWith("]"))
			{
				index = path.first().mid(1, path.first().size() - 2).toInt();
				path.removeFirst();
			}

			for (int i = 0; i < sourceArray.size(); ++i)
			{
				QJsonValue const element = sourceArray[i];
				QJsonValue modifiedElement = element;

				if (i == index)
				{
					modifyValue(element, modifiedElement, path, newValue);
				}

				targetArray.append(modifiedElement);
			}

			// If we're appending a new value outside bounds (e.g., into an empty array)
			if (sourceArray.isEmpty() && index == 0 && newValue != QJsonValue::Null)
			{
				targetArray.append(newValue);
			}

			target = targetArray;
		}

		// Handle primitive values
		else
		{
			if (path.isEmpty() && newValue != QJsonValue::Null)
			{
				target = newValue;
			}
			else
			{
				target = source;
			}
		}
	}
};
