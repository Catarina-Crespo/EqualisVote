import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import { Icon } from "@iconify/react";
import { loadWasmFunctions, generateGroupTag, confirmGroup } from '../backend/wasmAPI';
import { useUser } from '../context/UserContext';
import ElectionVoteModal from './modals/ElectionVoteModal';

const group_type = "Group Invite";
const election_invite = "Vote on Election";

const Notification = ({ isGroupInvite, data, onRemove, refresh }) => {

    const [isElectionVoteModalOpen, setIsElectionVoteModalOpen] = useState(false);
    const [tag, setTag] = useState(null);

    const { keys } = useUser();

    //console.log("isGroupInvite: " + isGroupInvite);


    const handleVoteSubmit = (voteData) => {
        console.log("Vote submitted:", voteData);
        // TODO: send to backend with axios.post("/send-vote", voteData)
    };

    const rejectInvite = () => {
        confirmGroup(data.groupId, "no", keys.upk, data.ID);
        console.log('Invite was rejected!');
        onRemove(data.ID);
    };

    const confirmInvite = async () => {
        //const wasm = await loadWasmFunctions();
        //const genTag = await generateGroupTag(wasm, data.groupId, keys.upk, keys.v_i);
        //setTag("empty");
        confirmGroup(data.groupId, "yes", keys.upk, data.ID);
        console.log('Invite was confirmed!');
        onRemove(data.ID);
    };

    const voteOnElection = () => {
        alert('You voted on this election!');
        //onRemove(data.ID);
    };

    let notif_type;
    let notif_message;

    if (isGroupInvite == "group_invite") {
        notif_type = group_type;
        notif_message = `You were added to the group "${data.name}". Do you confirm you want to join this group?`;
    } else {
        notif_type = election_invite;
        notif_message = `The election "${data.name}" was called.\n`;
        console.log(data);
    }

    return (
        <div className='notification-container'>
            <h3>{notif_type}</h3>
            <div>{notif_message}</div>
            <p>Group ID: {data.groupId}</p>
            {isGroupInvite == "group_invite" ? (
                <div className='notification-buttons'>
                    <button onClick={rejectInvite} className='icon-button reject-btn'><Icon icon="radix-icons:cross-2" width="20" height="20" className='button-icon' />Reject Invitation</button>
                    <button onClick={confirmInvite} className='icon-button accept-btn'><Icon icon="ic:round-check" width="20" height="20" className='button-icon' />Confirm and Join Group</button>
                </div>
            ) : (
                <div className='notification-buttons'>
                    <button onClick={() => setIsElectionVoteModalOpen(true)} className='icon-button'><Icon icon="streamline-ultimate:pencil-1-bold" width="16" height="16" className='button-icon' />Cast Vote</button>
                </div>)}
            <ElectionVoteModal
                isOpen={isElectionVoteModalOpen}
                onClose={() => {setIsElectionVoteModalOpen(false);}}
                onSubmit={handleVoteSubmit}
                election={data}
                handleRemove={() => onRemove(data.ID)}
            />
        </div>
    );
};

export default Notification;
