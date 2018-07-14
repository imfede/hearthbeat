import React, { Component } from 'react';
import ChartistGraph from 'react-chartist';
import moment from 'moment';
import $ from 'jquery';

class Target extends Component {
    constructor() {
        super();
        this.state = {
            target: {},
            targetInfo: {},
            data: []
        };
    }

    getTargetData(target) {
        $.get({
            url: `http://localhost:4000/api/clocks/${target.id}`,
            dataType: 'json',
            data: { reverse: true }
        }).then(data => this.setState({ data: data.reverse() }));
    }

    getTargetInfo(target) {
        $.get({
            url: `http://localhost:4000/api/clocks/${target.id}/count`,
            dataType: 'json'
        }).then(data => this.setState({ targetInfo: data }));
    }

    setTarget(target) {
        this.getTargetInfo(target);
        this.getTargetData(target);
        this.setState({ target: target });
    }

    render() {
        let chartData = this.state.data;

        let data = {
            labels: chartData.map(d => d.time),
            series: [chartData.map(d => (d.s * 1e9 + d.ns) / 1e9)]
        }

        let labelStep = Math.round(data.labels.length / 10);

        let options = {
            low: 0,
            showArea: true,
            axisX: {
                labelOffset: {
                    x: -20
                },
                labelInterpolationFnc: function (value, index) {
                    return index % labelStep === 0 ? moment(value, 'YYYY-MM-DD hh:mm:ss').format("hh:mm:ss") : null;
                }
            }
        }

        return (
            <div>
                <ChartistGraph className="ct-golden-section" data={data} options={options} type="Line" />
            </div>
        );
    }
}

export default Target;
