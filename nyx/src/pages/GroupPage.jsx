import React, { useState, useEffect } from 'react';
import { useParams } from 'react-router-dom';
import axios from 'axios';
import User from '../components/User';
import Election from '../components/Election';
import { Icon } from "@iconify/react";
import { getGroup } from '../backend/wasmAPI';
import { useUser } from '../context/UserContext';

const GroupPage = () => {
    const { spk } = useParams(); // Get spk from URL
    const [group, setGroup] = useState(null);
    const [copied, setCopied] = useState(false);

    const { keys } = useUser();

    
    useEffect(() => {
        if (!keys) return; // wait until keys are loaded

        const fetchGroup = async () => {
            try {
                const result = await getGroup(spk);
                setGroup(result);

                console.log("Fetched group:", result);

            } catch (error) {
                console.error("Error fetching group:", error);
            }
        };

        fetchGroup();
    }, [keys, spk]);

    const handleCopyGroupId = async () => {
        try {
            await navigator.clipboard.writeText(spk);
            setCopied(true);
            setTimeout(() => setCopied(false), 2000); // reset after 2s
        } catch (err) {
            console.error("Failed to copy Group ID:", err);
        }
    };

    const createGroup = () => {
        alert('Group Created!');
    };

    const createElection = () => {
        alert('Election Created!');
    };

    if (!group) return <p>Loading group...</p>;

    return (
        <div className="group-page">
            <h1>Group: {group.name}</h1>
            <p>ID: {spk}</p>

            <div className='group-page-subsection'>

                <div>

                    <h2>Group Elections</h2>

                    <h3>On Going Elections</h3>
                    {group.elections.length === 0 ? (
                        <p>No on going elections.</p>
                    ) : (
                        <ul className='groups-list'>
                            {group.elections.filter(election => election.result == null).map(org => (
                                <li key={org.id}><Election electionName={org.name} date={org.deadline} epk={org.epk_E} /></li>
                            ))}
                        </ul>
                    )}

                    <h3>Archived Elections</h3>
                    {group.elections.length === 0 ? (
                        <p>No archived elections.</p>
                    ) : (
                        <ul className='groups-list'>
                            {group.elections.filter(election => election.result != null).map(org => (
                                <li key={org.id}><Election electionName={org.name} date={org.deadline} epk={org.epk_E} /></li>
                            ))}
                        </ul>
                    )}

                </div>
                <div>
                    <h2>Group members</h2>
                    {group.users.length === 0 ? (
                        <p>No members in this group.</p>
                    ) : (
                        <ul className='members-list'>
                            {group.users.map(org => (
                                <li key={org.id}><User username={org.username} upk={org.upk} /></li>
                            ))}
                        </ul>
                    )}

                    <button onClick={handleCopyGroupId} className='icon-button'>
                        <Icon icon="mingcute:clipboard-fill" width="24" height="24" className='button-icon' />
                        {copied ? "Copied!" : "Copy Group ID"}
                    </button>

                </div>

            </div>
        </div>
    );
};

export default GroupPage;
