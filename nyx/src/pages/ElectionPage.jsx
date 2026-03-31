import React, { useState, useEffect } from 'react';
import { useParams } from 'react-router-dom';
import axios from 'axios';
import { Icon } from "@iconify/react";
import ElectionVoteModal from "../components/modals/ElectionVoteModal";
import { useUser } from '../context/UserContext';
import { formatDeadline, getElection, isElectionExpired } from '../backend/wasmAPI';

const ElectionPage = () => {
    const { epk } = useParams(); // Get epk from URL
    const [election, setElection] = useState(null);



    const { keys } = useUser();

    const [isElectionVoteModalOpen, setIsElectionVoteModalOpen] = useState(false);

    const handleVoteSubmit = (voteData) => {
        console.log("Vote submitted:", voteData);
        // TODO: send to backend with axios.post("/send-vote", voteData)
    };

    useEffect(() => {
        if (!keys) return; // wait until keys are loaded

        const fetchElection = async () => {
            try {
                const result = await getElection(epk);
                setElection(result);

                console.log("Fetched election:", result);

            } catch (error) {
                console.error("Error fetching groups:", error);
            }
        };

        fetchElection();
    }, [epk]);

    
    function formatElectionResult(resultStr, options) {
        if (!resultStr || resultStr == "null" || !options) return "The election hasn't finished or the result is being calculated. Please check it later.";

        let result;
        try {
            result = JSON.parse(resultStr); // parse "[3, 0, 0]" -> [3, 0, 0]
        } catch (err) {
            console.error("Invalid result format:", resultStr, err);
            return "Invalid result data";
        }

        const totalVotes = result.reduce((sum, n) => sum + n, 0);
        if (totalVotes === 0) {
            return "No votes have been cast yet.";
        }

        // Pair each option with its count and percentage
        const percentages = options.map((opt, idx) => {
            const count = result[idx] || 0;
            const pct = ((count / totalVotes) * 100).toFixed(1); // 1 decimal place
            return { option: opt, count, pct: parseFloat(pct) };
        });

        // Sort so winner is first
        const sorted = [...percentages].sort((a, b) => b.pct - a.pct);

        // Winner string
        const winner = sorted[0];

        // Breakdown string
        const breakdown = percentages
            .map((p) => `${p.option} = ${p.pct}%`)
            .join(", ");

        return `${winner.option} - ${winner.pct}% (${breakdown})`;
    }


    if (!election) return <p>Loading election...</p>;

    const handleVote = () => {
        alert(`Voting on election: ${election.name}`);
        // TODO: open VoteModal or send vote
    };

    return (
        <div className="group-page">

            <h1>{election.name}</h1>

            <p><strong>Election ID:</strong> {election.epk_E}</p>
            <p><strong>Deadline: </strong>{formatDeadline(election.deadline)}</p>

            <p><strong>Bulletin Board:</strong></p>
            <ul style={{ marginLeft: "20px" }}>
                {election.votes.map(org => (
                    <li>{org}</li>

                ))}
                <ElectionVoteModal
                    isOpen={isElectionVoteModalOpen}
                    onClose={() => setIsElectionVoteModalOpen(false)}
                    onSubmit={handleVoteSubmit}
                    election={election}
                />
            </ul>

            {!isElectionExpired(election.deadline) &&

                <div className='election-page-subsection' style={{ marginTop: "10px" }}>


                    <button onClick={() => setIsElectionVoteModalOpen(true)} className='icon-button'>
                        <Icon icon="mdi:ballot" width="24" height="24" className='button-icon' />
                        Vote
                    </button>
                </div>
            }



            <h2 data-testid="election-result" style={{ margin: '60px 0' }}>
                {election.result !== null ? (
                    <>
                        <strong>Result: </strong>{formatElectionResult(election.result, election.options)}
                    </>
                ) : (
                    "The election hasn't finished or the result is being calculated. Please check it later."
                )}
            </h2>

        </div>


    );
};

export default ElectionPage;
