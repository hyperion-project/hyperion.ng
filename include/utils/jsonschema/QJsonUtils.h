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

	/// @brief Recursively constructs a default JSON value from a JSON-Schema node.
	///
	/// The function walks the schema tree and assembles a concrete JSON value that
	/// satisfies the schema's structure, populating fields with their declared
	/// @c default values wherever available.
	///
	/// @par Top-level typed node (schema contains a @c "type" key)
	/// - @c "object" — recurses into the schema's @c "properties" sub-object to
	///   build a composed @c QJsonObject, but only when the field is required.
	/// - @c "array"  — uses the schema's @c "default" array if present; otherwise
	///   recurses into @c "items" and appends one element when it resolves to a
	///   non-empty object, but only when the field is required.
	/// - Any other scalar type — returns the @c "default" value when present and
	///   the field is required, or @c QJsonValue::Null otherwise.
	///
	/// @par Property-bag node (schema has no top-level @c "type" key)
	/// Each key/value pair in the object is treated as a named schema property and
	/// is processed with the same object / array / scalar rules above.  The results
	/// are collected into a single @c QJsonObject and returned.
	///
	/// @param schema        The JSON-Schema node to evaluate.  May be a typed
	///                      schema object or a flat property-bag.
	/// @param ignoreRequired When @c true every property is treated as if it were
	///                      marked @c "required": true, so default values are
	///                      emitted even for optional fields.
	/// @return A @c QJsonValue that represents the default structure described by
	///         @p schema, or @c QJsonValue::Null / an empty object when no
	///         meaningful value can be derived.
	static QJsonValue createValue(const QJsonValue& schema, bool ignoreRequired)
	{
		QJsonObject composedObject;
		QJsonObject const obj = schema.toObject();

		// Handle top-level schema node that carries an explicit "type" field.
		// Only one value is produced and returned immediately from this branch.
		auto typeIt = obj.constFind("type");
		if (typeIt != obj.constEnd() && typeIt->isString())
		{
			QString const typeStr = typeIt->toString();
			// A field is "active" when it is explicitly required or we are in
			// ignore-required mode (e.g. building a full default document).
			bool const isRequired = obj.value("required").toBool() || ignoreRequired;
			QJsonValue finalValue = QJsonValue::Null;

			if (typeStr == "object" && isRequired)
			{
				// Recurse into the nested properties map to build the child object.
				finalValue = createValue(obj.value("properties"), ignoreRequired);
			}
			else if (typeStr == "array" && isRequired)
			{
				if (obj.contains("default"))
				{
					// Schema provides a ready-made default array — use it directly.
					finalValue = obj.value("default");
				}
				else
				{
					// Build an array from the "items" sub-schema.  Only append the
					// synthesised element when it resolves to a non-empty object;
					// primitive item types are left as an empty array.
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
				// Scalar types (string, number, boolean, …): use the declared default.
				if (obj.contains("default"))
				{
					finalValue = obj.value("default");
				}
				// No default defined — leave finalValue as Null.
			}

			return finalValue;
		}

		// No top-level "type" key: treat the object as a flat map of named
		// schema properties and compose them into a single result object.
		for (QJsonObject::const_iterator it = obj.constBegin(); it != obj.constEnd(); ++it)
		{
			QString const attribute = it.key();
			const QJsonValue& attributeValue = it.value();
			QJsonObject const attrObj = attributeValue.toObject();

			// Determine the type and requiredness of this individual property.
			auto attrTypeIt = attrObj.constFind("type");
			QString const attrType = (attrTypeIt != attrObj.constEnd()) ? attrTypeIt->toString() : QString();
			bool const attrIsRequired = attrObj.value("required").toBool() || ignoreRequired;

			if (!attrType.isEmpty())
			{
				if (attrType == "object" && attrIsRequired)
				{
					// Prefer a sibling "properties" key on the parent; fall back to
					// recursing the attribute's own schema value.
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
						// Use the schema-declared default array.
						composedObject.insert(attribute, attrObj.value("default"));
					}
					else
					{
						// Synthesise an array from the "items" sub-schema.
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
					// Scalar property: emit the default or an explicit Null sentinel.
					if (attrObj.contains("default"))
					{
						composedObject.insert(attribute, attrObj.value("default"));
					}
					else
					{
						composedObject.insert(attribute, QJsonValue::Null);
					}
				}
				// Non-required properties with no default are silently omitted.
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

					//Ignore elements with null values
					if (subTarget != QJsonValue::Null)
					{
						targetObj.insert(key, subTarget);
					}

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
				//Do not add elements being null
				if (newValue != QJsonValue::Null)
				{
					target = source;
				}
			}
		}
	}
};
