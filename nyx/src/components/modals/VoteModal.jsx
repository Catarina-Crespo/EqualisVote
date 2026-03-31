import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import { useUser } from '../../context/UserContext';
import { encryptVote, loadWasmFunctions, sendBallot } from '../../backend/wasmAPI';

const VoteModal = ({ isOpen, onClose, onSubmit }) => {
    const { keys } = useUser();

    const [votee, setVotee] = useState('');
    const [score, setScore] = useState('');
    const [loading, setLoading] = useState(false);
    const [encrypted, setEncrypted] = useState(false);
    const [sent, setSent] = useState(false);

    const [ballot, setBallot] = useState(null);
        
        
    if (!isOpen) return null; // don't render if modal is closed

    const handleEncrypt = async () => {
        const wasm = await loadWasmFunctions();

        if (!votee) {
            alert("Please fill in both fields");
            return;
        }

        if (!score) {
            alert("Score must be between 1 and 10");
            return;
        }

        const numericScore = Number(score);
        if (numericScore < 1 || numericScore > 10) {
            alert("Score must be between 1 and 10");
            return;
        }
        
        const encryptedBallot = await encryptVote(keys, votee, wasm, numericScore);
        console.log(encryptedBallot);
        setBallot(encryptedBallot);

        setLoading(true);

        // simulate encryption taking 2s
        setTimeout(() => {
            setLoading(false);
            setEncrypted(true);
        }, 2000);
    };

    const handleSend = () => {
        setSent(true);

        sendBallot(votee, ballot, true);

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
                <div className='modal-header'>
                    <h2>Vote on User</h2>
                    <button onClick={onClose}>
                        <Icon icon="radix-icons:cross-2" width="24" height="24" />
                    </button>
                </div>

                {!encrypted && !sent && (
                    <>
                        <label>
                            Insert the username or the user's public ID:
                            <input
                                type="text"
                                value={votee}
                                onChange={(e) => setVotee(e.target.value)}
                            />
                            <p className='input-help'>
                                <Icon icon="material-symbols:help-rounded" width="12" height="12" className='help-icon' />
                                e.g. @john_doe or 84963f19fc...
                            </p>
                        </label>

                        <label>
                            Insert the reputation value you associate with this user:
                            <input
                                type="number"
                                min={1}
                                max={10}
                                value={score}
                                onChange={(e) => setScore(e.target.value)}
                            />
                            <p className='input-help'>
                                <Icon icon="material-symbols:help-rounded" width="12" height="12" className='help-icon' />
                                Choose a number between 1 and 10
                            </p>
                        </label>
                    </>
                )}

                {encrypted && !sent && (
                    <div className="encrypted-message" style={{display:"flex", flexDirection:'column'}}>
                        <Icon icon="material-symbols:mail-lock" width="24" height="24" className='msg-icon' /> 
                        Your vote has been protected! Click below to send the cyphered value to the server.<br />
                        Your protected ballot: <code style={{marginTop:'10px'}}>{ballot.substring(0,30)}...</code>
                        
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
                    ) : encrypted && !sent ? (
                        <button onClick={handleSend}>Send Vote</button>
                    ) : !encrypted && !sent ? (
                        <button onClick={handleEncrypt}>Protect Vote</button>
                    ) : null}
                </div>
            </div>
        </div>
    );
};

export default VoteModal;
