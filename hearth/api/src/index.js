const path = require('path');
const express = require('express');
const cors = require('cors');
const TargetRepository = require('./TargetRepository');
const ClocksRepository = require('./ClocksRepository');

const app = express();

const router = express.Router();

const targetRepository = new TargetRepository("/var/hearthbeat/database.db");
const clocksRepository = new ClocksRepository("/var/hearthbeat/database.db");

router.get('/targets', async function (req, res) {
    targetRepository.getTargets().then(rows => res.json(rows));
});

router.get('/targets/:id', async function (req, res) {
    targetRepository.getTarget(req.params.id).then(target => res.json(target));
});

router.get('/clocks/count', function (req, res) {
    clocksRepository.countAllClocks().then(clocks => res.json(clocks));
});

router.get('/clocks/:id', function (req, res) {
    let options = {
        id: req.params.id,
        limit: req.query.limit,
        offset: req.query.offset,
        reverse: req.query.reverse
    };
    clocksRepository.getClocks(options).then(clocks => res.json(clocks));
});

router.get('/clocks/:id/count', function (req, res) {
    clocksRepository.countClocks(req.params.id).then(count => res.json(count));
});

app.use(cors());
app.use('/api', router);
app.use(express.static(path.join(__dirname, '../static')));

app.listen(4000);