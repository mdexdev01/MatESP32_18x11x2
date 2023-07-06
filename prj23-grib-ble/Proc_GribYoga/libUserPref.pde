

int [][] pref_sensor_dist_max;
boolean pref_z_depth = false;
boolean pref_show_ghost = false;


class UserPreference {
    StringDict prefDict;
    ListBox pParentLogBox = null;
    boolean isLog = false;
    Table       table;
    String      table_path;

    String itemPort = "use_port#1";
    boolean isLoaded = false;

    UserPreference() {
        prefDict = new StringDict();
    }

    String getValue(String key) {
        return prefDict.get(key);
    }

    void setValue(String key, String value) {
        prefDict.set(key, value);
    }

    boolean loadTable_CSV(String csv_path) {
        boolean result = true;
        table_path = csv_path;
        table = new Table();
        
        try {
            table = loadTable(csv_path, "header");
        } catch (Exception e) {
            logAppend(e.toString());
            result = false;
        }

        int row_count = table.getRowCount();
        for (TableRow row : table.rows()) {
            prefDict.set(row.getString("item"), row.getString("value"));
            println(row.getString("item") + row.getString("value"));
        }

        isLoaded = result;
        return result;
    }

    boolean saveTable_CSV() {
        if(isLoaded == false)
            return false;

        Table tableSave;
        tableSave = new Table();
        
        tableSave.addColumn("item");
        tableSave.addColumn("value");
        
        TableRow newRow = tableSave.addRow();
        newRow.setString("item", itemPort);
        newRow.setString("value", prefDict.get(itemPort));
        
        newRow = tableSave.addRow();
        newRow.setString("item", "test1");
        newRow.setString("value", prefDict.get("test1"));

        newRow = tableSave.addRow();
        newRow.setString("item", "test2");
        newRow.setString("value", prefDict.get("test2"));

        saveTable(tableSave, table_path);
        return true;
    }

    void configLogBox(ListBox listLog) {
        pParentLogBox = listLog;
        isLog = true;
    }

    void logAppend(String logMsg) {
        if( isLog == false )
            return;

        String message = "[Preference] " + logMsg;
        pParentLogBox.addItem(message, 0);
        println(message);
        
        return;
    }
}

//------------------------------
//  Preference
UserPreference userPref;
String csvPathPref = "./res/Preference.csv";


void setup_Pref() {
  userPref = new UserPreference();
  if ( false == userPref.loadTable_CSV(csvPathPref) ) {
    println("Failed to load CSV file. - " + csvPathPref);
    return;
  }
}
