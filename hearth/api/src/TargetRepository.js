const sqlite3 = require('sqlite3').verbose();

class TargetRepository {
    constructor(dbfile) {
        this.dbfile = dbfile;
    }

    _exec(callback) {
        let db = new sqlite3.Database(this.dbfile);
        callback.apply(this, [db]);
        db.close();
    }

    getTargets() {
        return new Promise((resolve, reject) => {
            this._exec(db => {
                const query = "SELECT id, name FROM targets;";
                db.all(query, (err, rows) => {
                    if( err === null) {
                        resolve(rows);
                    } else {
                        reject(err);
                    }
                });
            });
        });
    }

    getTarget(id) {
        return new Promise((resolve, reject) => {
            this.getTargets().then(rows => {
                const candidates = rows.filter(target => target.id == id);
                if(candidates.length !== 1) {
                    reject("Target not found");
                } else {
                    resolve(candidates[0]);
                }
            })
        });
    }
}

module.exports = TargetRepository;