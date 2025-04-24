## LibreSprite + MCP + Cursor

#### Bridge (MCP)

* npm run install
* npm run build
* lsof -i :8080
* find the service working with 8080 and kill it
* run the cursor (WARNING: if you are using Linux, Cursor does not able to enable mcp servers if you are not running it via terminal. Simply mark the Cursor.AppImage as executable and execute it from the terminal)

Currently each restart requires the step 3. Else there will be conflict via mcp server and cursor.

### Cursor MCP settings
```
"libresprite": {
      "command": "node",
      "args": ["/home/vviglaf/Desktop/Libre/bridge-server/dist/server.js"]
    }
```

### LibreSprite
* execute the build "/build/bin/libresprite"
* create a new project
* draw something simple
* click file from the toolbar and then "Send to bridge"

### Testing
* Back to the Cursor IDE
* First you can execute the command via your terminal to see your visual "node visualize.js"

** Tools: **
-  List Drawings
-  Clear Drawings
-  Analyze Drawing


Analyze drawing does not work properly currently (it was actually, but i broke it)

This study is quite dirty, but it was created for the purpose of providing perspective to usages of MCP etc.