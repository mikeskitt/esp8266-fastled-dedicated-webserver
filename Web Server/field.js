const fields = require('./fields.js')
const patterns = require('./patterns.js')

exports.getField = function(name) {
  for (let j = 0; j < fields.fieldsCount; j++) {
    field = fields.fields[j];
    if (field.name == name) {
      return field;
    }
  }
  return null;
}

exports.getFieldValue = function(name) {
  let field = this.getField(name);
  if (this.field.getValue != null) {
    return (field.getValue)();
  }
  return null;
}

exports.setFieldValue = function(name, value) {
  let field = this.getField(name);
  if (field.setValue) {
    return field.setValue(value);
  }
  return null;
}

exports.getFieldsJson = function() {
  let json = "[";
  let count = fields.fieldsCount;
  for (let i = 0; i < count; i++) {
    field = fields.fields[i];

    json += "{\"name\":\"" + field.name + "\",\"label\":\"" + field.label + "\",\"type\":\"" + field.type + "\"";
    if (field.type == fields.ColorFieldType || field.type == "String") {
      if (field.getValue != null) json += ",\"value\":\"" + field.getValue() + "\"";
      else json += ",\"value\":\"\"";
    }
    else {
      if (field.getValue != null) json += ",\"value\":" + field.getValue();
      else json += ",\"value\":\"\"";
    }

    if (field.type == fields.NumberFieldType) {
      json += ",\"min\":" + String(field.min);
      json += ",\"max\":" + String(field.max);
    }

    if (field.getOptions) {
      json += ",\"options\":[";
      json += field.getOptions();
      json += "]";
    }

    json += "}";

    if (i < count - 1)
      json += ",";
  }

  json += "]";

  return json;
}