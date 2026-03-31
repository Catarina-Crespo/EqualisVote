import React from 'react';
import { Link } from 'react-router-dom';
import { Icon } from "@iconify/react";

const Group = ({ groupName, groupSpk }) => {

    return (
        <Link to={`/groups/${groupSpk}`}>
            <div className='group-container'>
                <div>
                    <h3>{groupName}</h3>
                    <p>Group ID: {groupSpk}</p>
                </div>
                <Icon icon="bitcoin-icons:caret-right-filled" width="24" height="24" />
            </div>
        </Link>
    );
};

export default Group;
