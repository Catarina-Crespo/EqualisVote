import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import { encryptVote, sendBallot, loadWasmFunctions } from '../../backend/wasmAPI';
import { useUser } from '../../context/UserContext';

const ElectionVoteModal = ({ isOpen, onClose, election, onSubmit, handleRemove }) => {
    const [selectedOption, setSelectedOption] = useState('');
    const [loading, setLoading] = useState(false);
    const [encrypted, setEncrypted] = useState(false);
    const [sent, setSent] = useState(false);
    const [encryptedBallot, setEncryptedBallot] = useState('');

    const { keys } = useUser();

    console.log("Logging 'election': ")
    console.log(election);

    if (!isOpen || !election) return null;

    const handleEncrypt = async () => {
        const wasm = await loadWasmFunctions();

        if (!selectedOption) {
            alert("Please select an option before encrypting.");
            return;
        }

        if (!keys) {
            alert("You need to submit your keys");
            return;
        }

        let index = election.options.indexOf(selectedOption);

        const result = await encryptVote(election.epk_E, index);
        console.log("Logging encryption result: ");
        console.log(result);
        setEncryptedBallot(result);

        setLoading(true);
        setTimeout(() => {
            setLoading(false);
            setEncrypted(true);
        }, 1000); // simulate encryption delay
    };

    const handleSend = () => {
        // Mark the vote as sent before the modal closes
        setSent(true);
        console.log("Sending the encrypted ballot: " + encryptedBallot + " to the election " + election.epk_E)
        sendBallot(election.epk_E, encryptedBallot, false);

        handleRemove();

        // simulate sending to server
        setTimeout(() => {
            onSubmit({ votee, score });
            setVotee('');
            setScore('');
            setEncrypted(false);
            setSent(false);
            onClose();
        }, 4000);
    };


    return (
        <div className="modal-overlay">
            <div className="modal">
                <div className="modal-header">
                    <h2>Vote on: {election.name}</h2>
                    <button onClick={onClose}>
                        <Icon icon="radix-icons:cross-2" width="24" height="24" />
                    </button>
                </div>

                {!encrypted && !sent && (
                    <>
                        <p>Please choose one option:</p>
                        <div className="options-list">
                            {election.options.map((opt, idx) => (
                                <div className='option-div'>
                                    <input
                                        type="radio"
                                        id={idx}
                                        value={opt}
                                        checked={selectedOption === opt}
                                        onChange={(e) => setSelectedOption(e.target.value)}

                                    />
                                    <label key={idx} for={idx} className="option-item">{opt}</label>
                                </div>

                            ))}
                        </div>
                    </>
                )}

                {encrypted && !sent && (
                    <div className="encrypted-message" style={{ display: "flex", flexDirection: 'column' }}>
                        <Icon icon="material-symbols:mail-lock" width="24" height="24" className='msg-icon' />
                        Your vote has been protected! Click below to send the cyphered value to the server.<br />
                        Your protected ballot: <code style={{ marginTop: '10px' }}>{encryptedBallot.substring(0, 30)}...</code>

                    </div>
                )}

                {sent && (
                    <div className="sent-animation">
                        <p><Icon icon="line-md:circle-to-confirm-circle-transition" width="24" height="24" className="success-icon" /> Your vote has been sent!</p>
                    </div>
                )}



                <div className="modal-actions">
                    {loading ? (
                        <div className="loading-spinner">
                            <Icon icon="eos-icons:loading" width="28" height="28" />
                            <span>Anonymizing...</span>
                        </div>
                    ) : encrypted ? (
                        <button onClick={handleSend}>Send Vote</button>
                    ) : (
                        <button onClick={handleEncrypt}>Protect Vote</button>
                    )}
                </div>
            </div>
        </div>
    );
};

export default ElectionVoteModal;
