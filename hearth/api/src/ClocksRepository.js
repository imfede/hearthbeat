const sqlite3 = require('sqlite3');

class ClocksRepository {
    constructor(dbfile) {
        this.dbfile = dbfile;
    }

    _exec(callback) {
        let db = new sqlite3.Database(this.dbfile);
        callback.apply(this, [db]);
        db.close();
    }

    getClocks({ id, limit = 100, offset = 0, reverse = false }) {
        return new Promise((resolve, reject) => {
            this._exec(db => {
                let order = reverse ? "DESC" : "ASC";
                const query = `SELECT target, time, s, ns FROM clocks WHERE target = ? ORDER BY time ${order} LIMIT ? OFFSET ? ;`;
                const bindings = {
                    1: id,
                    2: limit,
                    3: offset
                };
                db.all(query, bindings, (err, rows) => {
                    if (err === null) {
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
                    if (err === null) {
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
                    if (err === null) {
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