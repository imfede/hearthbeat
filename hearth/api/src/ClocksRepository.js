const sqlite3 = require('sqlite3').verbose();

class ClocksRepository {
    constructor(dbfile) {
        this.dbfile = dbfile;
    }

    _exec(callback) {
        let db = new sqlite3.Database(this.dbfile);
        callback.apply(this, [db]);
        db.close();
    }

    getClocks(targetId, limit=100, offset=0) {
        return new Promise((resolve, reject) => {
            this._exec(db => {
                const query = "SELECT target, time, s, ns FROM clocks WHERE target = ? LIMIT ? OFFSET ?;";
                const bindings = {
                    1: targetId,
                    2: limit,
                    3: offset
                };
                db.all(query, bindings, (err, rows) => {
                    if( err === null) {
                        resolve(rows);
                    } else {
                        reject(err);
                    }
                });
            });
        });
    }

    countClocks(targetId) {
        return new Promise((resolve, reject) => {
            this._exec(db => {
                const query = "SELECT target, count(*) as count FROM clocks WHERE target = ?;";
                const bindings = { 1: targetId };
                db.all(query, bindings, (err, rows) => {
                    if( err === null) {
                        resolve(rows[0]);
                    } else {
                        reject(err);
                    }
                });
            });
        });
    }

    countAllClocks() {
        return new Promise((resolve, reject) => {
            this._exec(db => {
                const query = "SELECT target, count(*) as count FROM clocks GROUP BY target;";
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
}

module.exports = ClocksRepository;