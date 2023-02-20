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

	static void modify(QJsonObject& value, QStringList path, const QJsonValue& newValue = QJsonValue::Null, QString propertyName = "")
	{
		QJsonObject result;

		if (!path.isEmpty())
		{
			if (path.first() == "[root]")
				path.removeFirst();

			for (QStringList::iterator it = path.begin(); it != path.end(); ++it)
			{
				QString current = *it;
				if (current.left(1) == ".")
					*it = current.mid(1, current.size()-1);
			}

			if (!value.isEmpty())
				modifyValue(value, result, path, newValue, propertyName);
			else if (newValue != QJsonValue::Null && !propertyName.isEmpty())
				result[propertyName] = newValue;
		}

		value = result;
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

	static void modifyValue(QJsonValue source, QJsonObject& target, QStringList path, const QJsonValue& newValue, QString& property)
	{
		QJsonObject obj = source.toObject();

		if (!obj.isEmpty())
		{
			for (QJsonObject::iterator i = obj.begin(); i != obj.end(); ++i)
			{
				QString propertyName = i.key();
				QJsonValue subValue = obj[propertyName];

				if (subValue.isObject())
				{
					if (!path.isEmpty())
					{
						if (propertyName == path.first())
						{
							path.removeFirst();

							if (!path.isEmpty())
							{
								QJsonObject tempObj;
								modifyValue(subValue, tempObj, path, newValue, property);
								subValue = tempObj;
							}
							else if (newValue != QJsonValue::Null)
								subValue = newValue;
							else
								continue;

							if (!subValue.toObject().isEmpty())
								target[propertyName] = subValue;
						}
						else
						{
							if (path.first() == property && newValue != QJsonValue::Null)
							{
								target[property] = newValue;
								property = QString();
							}

							target[propertyName] = subValue;
						}
					}
					else
						if (!subValue.toObject().isEmpty())
							target[propertyName] = subValue;
				}
				else if (subValue.isArray())
				{
					if (!path.isEmpty())
					{
						if (propertyName == path.first())
						{
							path.removeFirst();

							int arrayLevel = -1;
							if (!path.isEmpty())
							{
								if ((path.first().left(1) == "[") && (path.first().right(1) == "]"))
								{
									arrayLevel = path.first().mid(1, path.first().size()-2).toInt();
									path.removeFirst();
								}
							}

							QJsonArray array = subValue.toArray();
							QJsonArray json_array;

							for (QJsonArray::iterator i = array.begin(); i != array.end(); ++i)
							{
								if (!path.isEmpty())
								{
									QJsonObject arr;
									modifyValue(*i, arr, path, newValue, property);
									subValue = arr;
								}
								else if (newValue != QJsonValue::Null)
									subValue = newValue;
								else
									continue;

								if (!subValue.toObject().isEmpty())
									json_array.append(subValue);
								else if (newValue != QJsonValue::Null && arrayLevel != -1)
									json_array.append( (i - array.begin() == arrayLevel) ? subValue : *i );
							}

							if (!json_array.isEmpty())
								target[propertyName] = json_array;
							else if (newValue != QJsonValue::Null && arrayLevel == -1)
								target[propertyName] = newValue;
						}
						else
						{
							if (path.first() == property && newValue != QJsonValue::Null)
							{
								target[property] = newValue;
								property = QString();
							}

							target[propertyName] = subValue;
						}
					}
					else
						if (!subValue.toArray().isEmpty())
							target[propertyName] = subValue;
				}
				else
				{
					if (!path.isEmpty())
					{
						if (propertyName == path.first())
						{
							path.removeFirst();

							if (path.isEmpty())
							{
								if (newValue != QJsonValue::Null && property.isEmpty())
									subValue = newValue;
								else
									continue;
							}

							target[propertyName] = subValue;
						}
						else
						{
							if (path.first() == property && newValue != QJsonValue::Null)
							{
								target[property] = newValue;
								property = QString();
							}

							target[propertyName] = subValue;
						}
					}
					else
						target[propertyName] = subValue;
				}
			}
		}
		else if (newValue != QJsonValue::Null && !property.isEmpty())
		{
			target[property] = newValue;
			property = QString();
		}
	}
};
