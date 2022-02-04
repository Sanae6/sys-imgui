const fs = require("fs");

const readJson = (name) => {
    return JSON.parse(fs.readFileSync(name, "utf-8"))
}

