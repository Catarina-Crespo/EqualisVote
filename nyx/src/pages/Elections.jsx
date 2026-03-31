import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Icon } from "@iconify/react";
import Election from '../components/Election';
import CreateElectionModal from '../components/modals/CreateElectionModal';
import { getUserElections } from '../backend/wasmAPI';
import { useUser } from '../context/UserContext';

const Elections = () => {
  const [elections, setElections] = useState([]);
  const [isCreateElectionModalOpen, setIsCreateElectionModalOpen] = useState(false);

  const { keys } = useUser();

  const handleVoteSubmit = (voteData) => {
    console.log("Vote submitted:", voteData);
    // TODO: send to backend with axios.post("/send-vote", voteData)
  };

  const handleRefresh = () => {
    fetchElections();
  }

  const fetchElections = async () => {
    try {
      const result = await getUserElections(keys.upk);
      console.log("Logging result:")
      console.log(result);

      if (result.size > 0 && result.elections) {
        setElections(result.elections);
        console.log(result.elections)
      }

      console.log("Fetched elections:", result.size);

    } catch (error) {
      console.error("Error fetching groups:", error);
    }
  };

  useEffect(() => {
    if (!keys) return; // wait until keys are loaded
    fetchElections();
  }, [keys]); // re-run when keys change


  const createElection = () => {
    alert('Election Created!');
  };

  return (
    <div className="elections-section">
      <div className='header'>
        <h1>Your Elections</h1>
        <button onClick={handleRefresh} name='refresh' data-testid="refresh"><Icon icon="material-symbols:refresh-rounded" width="32" height="32" /></button>
      </div>
      <div className='buttons-container'>
        <button onClick={() => setIsCreateElectionModalOpen(true)} className='icon-button'><Icon icon="streamline-ultimate:pencil-1-bold" width="20" height="20" className='button-icon' />Create Election</button>
      </div>
      {elections.length === 0 ? (
        <p>No elections in your groups.</p>
      ) : (
        <ul className='groups-list'>
          {elections.map(org => (
            <li key={org.epk}><Election electionName={org.name} date={org.deadline} epk={org.epk} spk={org.spk} group={org.groupName} /></li>
          ))}
        </ul>

      )}

      <CreateElectionModal
        isOpen={isCreateElectionModalOpen}
        onClose={() => setIsCreateElectionModalOpen(false)}
        onSubmit={handleRefresh}
      />
    </div>
  );
};

export default Elections;
