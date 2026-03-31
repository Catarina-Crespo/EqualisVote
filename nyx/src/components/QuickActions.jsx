import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import VoteModal from './modals/VoteModal';
import CreateGroupModal from './modals/CreateGroupModal';
import CreateElectionModal from './modals/CreateElectionModal';
import JoinGroupModal from './modals/JoinGroupModal';

const QuickActions = ({ onCreateGroup, onCreateElection }) => {

    const [isCreateGroupModalOpen, setIsCreateGroupModalOpen] = useState(false);
    const [isCreateElectionModalOpen, setIsCreateElectionModalOpen] = useState(false);
    const [isVoteModalOpen, setIsVoteModalOpen] = useState(false);
    const [isJoinGroupModalOpen, setIsJoinGroupModalOpen] = useState(false);

    const handleVoteSubmit = (voteData) => {
        console.log("Vote submitted:", voteData);
        // TODO: send to backend with axios.post("/send-vote", voteData)
    };



    return (
        <div className="quick-actions">
            <h3>Quick Actions</h3>
            <button onClick={() => setIsCreateGroupModalOpen(true)}><Icon icon="ic:round-group-add" width="24" height="24" className='button-icon' />Create Group</button>
            <button onClick={() => setIsCreateElectionModalOpen(true)}><Icon icon="streamline-ultimate:pencil-1-bold" width="24" height="24" className='button-icon' />Create Election</button>
            <button onClick={() => setIsVoteModalOpen(true)}><Icon icon="mdi:vote" width="24" height="24" className='button-icon' />Vote on User</button>
            <button onClick={() => setIsJoinGroupModalOpen(true)}><Icon icon="heroicons-solid:link" width="24" height="24" className='button-icon' />Join Group</button>


            <VoteModal
                isOpen={isVoteModalOpen}
                onClose={() => {setIsVoteModalOpen(false); }}
                onSubmit={handleVoteSubmit}
            />

            <CreateElectionModal
                isOpen={isCreateElectionModalOpen}
                onClose={() => setIsCreateElectionModalOpen(false)}
                onSubmit={onCreateElection}
            />

            <CreateGroupModal
                isOpen={isCreateGroupModalOpen}
                onClose={() => setIsCreateGroupModalOpen(false)}
                onSubmit={onCreateGroup}
            />

            <JoinGroupModal
                isOpen={isJoinGroupModalOpen}
                onClose={() => setIsJoinGroupModalOpen(false)}
                onSubmit={handleVoteSubmit}
            />


        </div>
    );
};

export default QuickActions;
