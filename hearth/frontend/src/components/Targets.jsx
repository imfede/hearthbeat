import React, { Component } from 'react';
import PropTypes from 'prop-types';
import $ from 'jquery';
import { Collection, CollectionItem, Input } from 'react-materialize';


class Targets extends Component {
    constructor() {
        super();
        this.state = {
            filtered: [],
            targets: []
        }
    }

    updateStates() {
        $.get({
            url: "/api/targets",
            dataType: "json"
        }).done(data => {
            this.setState({ targets: data, filtered: data });
        });
    }

    filterTargets(query) {
        if (query === "") {
            this.setState({ filtered: this.state.targets });
        } else {
            this.setState({ filtered: this.state.targets.filter(t => t.name.includes(query)) });
        }
    }

    componentWillMount() {
        this.updateStates();
    }

    handleClick(target) {
        this.props.onTargetSelect(target);
    }

    handleSearch(event, query) {
        this.filterTargets(query);
    }

    render() {
        const targets = this.state.filtered.map(target =>
            <CollectionItem key={target.id} onClick={this.handleClick.bind(this, target)}>
                {target.name}
            </CollectionItem>
        );

        return (
            <div>
                <Input label="Search" s={12} onChange={this.handleSearch.bind(this)} ref="search" />
                <Collection s={12}>
                    {targets}
                </Collection>
            </div>
        );
    }
}

Targets.propTypes = {
    onTargetSelect: PropTypes.func.isRequired
}

export default Targets;