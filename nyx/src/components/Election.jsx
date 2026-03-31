import React from 'react';
import { Link } from 'react-router-dom';
import { Icon } from "@iconify/react";
import { formatDeadline, getElection, isElectionExpired } from '../backend/wasmAPI';

const Election = ({ electionName, date, epk, spk }) => {

    return (
        <Link to={`/elections/${epk}`}>
            <div className='group-container'>
                <div>
                    <h3>{electionName}</h3>
                    <p><b>Deadline:</b> {formatDeadline(date)}</p>
                    <p><b>Election ID:</b> {epk.substring(0, 25)}...</p>
                    {spk && <p><b>Group ID:</b> {spk}</p>}
                </div>
                <Icon icon="bitcoin-icons:caret-right-filled" width="24" height="24" />
            </div>
        </Link>
    );
};

export default Election;
