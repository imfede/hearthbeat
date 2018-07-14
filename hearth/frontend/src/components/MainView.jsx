import React, { Component } from 'react';
import { Navbar, Row, Col } from 'react-materialize';

import Targets from './Targets';
import Target from './Target';

class MainView extends Component {

    handleTargetSelect(target) {
        this.refs.target.setTarget(target);
    }

    render() {
        return (
            <div>
                <Navbar brand='Hearth' right />
                <div className="content">
                    <Row>
                        <Col s={3} className="menu"><Targets onTargetSelect={this.handleTargetSelect.bind(this)} /></Col>
                        <Col s={9} className="boh"><Target ref="target"/></Col>
                    </Row>
                </div>
            </div>
        );
    }
}

export default MainView;