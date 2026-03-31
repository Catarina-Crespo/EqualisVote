import React, { useState, useEffect } from 'react';
import axios from 'axios';
import Group from '../components/Group';
import { Icon } from "@iconify/react";
import CreateGroupModal from '../components/modals/CreateGroupModal';
import JoinGroupModal from '../components/modals/JoinGroupModal';
import { getGroups } from '../backend/wasmAPI';
import { useUser } from '../context/UserContext';


const Groups = () => {

  const { keys } = useUser();

  const [groups, setGroups] = useState([]);
  const [isCreateGroupModalOpen, setIsCreateGroupModalOpen] = useState(false);
  const [isJoinGroupModalOpen, setIsJoinGroupModalOpen] = useState(false);

  const handleVoteSubmit = (voteData) => {
    console.log("Vote submitted:", voteData);
    // TODO: send to backend with axios.post("/send-vote", voteData)
  };

  const handleRefresh = () => {
    fetchGroups();
  }

  const fetchGroups = async () => {
    try {
      const result = await getGroups(keys.upk);
      //console.log("Logging result:")
      //console.log(result);

      if (result.size > 0 && result.groups) {
        setGroups(result.groups);
      }

      //console.log("Fetched groups:", result.size);

    } catch (error) {
      console.error("Error fetching groups:", error);
    }
  };

  useEffect(() => {
    if (!keys) return; // wait until keys are loaded   

    fetchGroups();
  }, [keys]); // re-run when keys change


  const createGroup = () => {
    alert('Group Created!');
  };

  const createElection = () => {
    alert('Election Created!');
  };

  return (
    <div className="groups-section">
      <div className='header'>
        <h1>Your Groups</h1>
        <button onClick={handleRefresh} name='refresh' data-testid="refresh"><Icon icon="material-symbols:refresh-rounded" width="32" height="32" /></button>
      </div>
      <div className='buttons-container'>
        <button onClick={() => setIsCreateGroupModalOpen(true)} className='icon-button'><Icon icon="ic:round-group-add" width="20" height="20" className='button-icon' />Create Group</button>
        <button onClick={() => setIsJoinGroupModalOpen(true)} className='icon-button'><Icon icon="heroicons-solid:link" width="20" height="20" className='button-icon' />Join Group</button>
      </div>
      {groups.length === 0 ? (
        <p>No groups found. You have to be part of a group to create an election.</p>
      ) : (
        <ul className='groups-list'>
          {groups.map(org => (
            <li key={org.spk}><Group groupName={org.name} groupSpk={org.spk} /></li>
          ))}
        </ul>
      )}

      <CreateGroupModal
        isOpen={isCreateGroupModalOpen}
        onClose={() => setIsCreateGroupModalOpen(false)}
        onSubmit={handleVoteSubmit}
      />

      <JoinGroupModal
        isOpen={isJoinGroupModalOpen}
        onClose={() => setIsJoinGroupModalOpen(false)}
        onSubmit={handleVoteSubmit}
      />
    </div>
  );
};

export default Groups;
