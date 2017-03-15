module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "text",
    "defaultValue": "personalize your experience"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Settings"
      },
      {
        "type": "input",
        "messageKey": "SelectedDate",
        "label": "Selected Date",
        "attributes": {
          "type": "date"
        }
      },
      {
        "type": "toggle",
        "messageKey": "hide_battery",
        "label": "Hide Battery State"
      },
      {
        "type": "toggle",
        "messageKey": "hide_date",
        "label": "Hide Date"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];