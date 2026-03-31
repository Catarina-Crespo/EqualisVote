import React from 'react';
import { Link } from 'react-router-dom';
import { Icon } from "@iconify/react";

const User = ({ username, upk }) => {

    return (
        <div className='user-container'>
            <div>
                <h3>{username}</h3>
                <p>{upk}</p>
            </div>
        </div>
    );
};

export default User;
