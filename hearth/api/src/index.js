const express = require('express');
const TargetRepository = require('./TargetRepository');
const ClocksRepository = require('./ClocksRepository');

const app = express();
const router = express.Router();

const targetRepository = new TargetRepository("/var/hearthbeat/database.db");
const clocksRepository = new ClocksRepository("/var/hearthbeat/database.db");

router.get('/targets', async function(req, res) {
    targetRepository.getTargets().then(rows => res.json(rows));
});

router.get('/targets/:id', async function(req, res) {
    targetRepository.getTarget(req.params.id).then(target => res.json(target));
});

router.get('/clocks/count', function(req, res) {
    clocksRepository.countAllClocks().then(clocks => res.json(clocks));
});

router.get('/clocks/:id', function(req, res) {
    clocksRepository.getClocks(req.params.id, req.query.limit, req.query.offset).then(clocks => res.json(clocks));
});

router.get('/clocks/:id/count', function(req, res) {
    clocksRepository.countClocks(req.params.id).then(count => res.json(count));
});

app.use('/api', router);

app.listen(4000);