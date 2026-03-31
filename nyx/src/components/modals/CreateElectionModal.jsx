import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import { createElection } from '../../backend/wasmAPI';

const CreateElectionModal = ({ isOpen, onClose, onSubmit }) => {
    const [members, setMembers] = useState('');
    const [loading, setLoading] = useState(false);
    const [submitted, setSubmitted] = useState(false);

    const [electionName, setElectionName] = useState('');
    const [electionDeadline, setElectionDeadline] = useState('');
    const [spk, setSpk] = useState('');
    const [options, setOptions] = useState('');

    if (!isOpen) return null;

    const handleSubmit = () => {
        if (!electionName.trim() || !electionDeadline || !spk.trim() || !options.trim()) {
            alert("Please fill in all the fields");
            return;
        }

        setLoading(true);

        const parsedOptions = options.split(",").map(m => m.trim());

        let election_info = {
            name: electionName,
            options: parsedOptions,
            deadline: electionDeadline,
            group_spk: spk
        }

        console.log(electionDeadline);
        let currentDate = new Date();
        console.log(new Date(currentDate.getTime() + 0.2 * 60 * 1000))

        console.log(election_info);

        createElection(election_info);

        if (onSubmit) {
            setTimeout(() => onSubmit(), 7000)
        }

        // simulate request taking 2s
        setTimeout(() => {
            setLoading(false);
            setSubmitted(true);

            // call parent onSubmit with parsed users
            onSubmit({ members: members.split(",").map(m => m.trim()) });

            // reset after 2s, then close modal
            setTimeout(() => {
                setElectionName('');
                setElectionDeadline('');
                setSpk('');
                setOptions('');
                setSubmitted(false);
                onClose();
            }, 8000);
        }, 2000);
    };

    return (
        <div className="modal-overlay">
            <div className="modal">
                <div className="modal-header">
                    <h2>Create Election</h2>
                    <button onClick={onClose}>
                        <Icon icon="radix-icons:cross-2" width="24" height="24" />
                    </button>
                </div>

                {!submitted ? (
                    <div>

                        <label>
                            Insert the ID of the group where you want to call this election:
                            <input
                                type="text"
                                value={spk}
                                onChange={(e) => setSpk(e.target.value)}
                            />
                            <p className='input-help'>
                                <Icon icon="material-symbols:help-rounded" width="12" height="12" className='help-icon' />
                                e.g. 84963f19fc...
                            </p>
                        </label>

                        <label>
                            Insert the question to be voted on:
                            <input
                                type="text"
                                value={electionName}
                                onChange={(e) => setElectionName(e.target.value)}
                            />
                            <p className='input-help'>
                                <Icon icon="material-symbols:help-rounded" width="12" height="12" className='help-icon' />
                                e.g. Do you approve the updates to the Internal Regulation?
                            </p>
                        </label>

                        <label>
                            Insert the available choices (comma-separated):
                            <input
                                type="text"
                                value={options}
                                onChange={(e) => setOptions(e.target.value)}
                            />
                            <p className='input-help'>
                                <Icon icon="material-symbols:help-rounded" width="12" height="12" className='help-icon' />
                                e.g. Yes, No, Blank, Invalid
                            </p>
                        </label>

                        <label>
                            Insert the election deadline:
                            <input
                                type="datetime-local"
                                value={electionDeadline}
                                onChange={(e) => setElectionDeadline(e.target.value)}
                            />
                        </label>



                    </div>

                ) : (
                    <div className="encrypted-message">
                        <p><Icon icon="line-md:circle-to-confirm-circle-transition" width="24" height="24" className="success-icon" /> <span>The election request was sent to the server!
                            You and the other group members will receive a notification soon with a request to cast a vote.</span></p>
                    </div>

                )}

                <div className="modal-actions">
                    {loading ? (
                        <div className="loading-spinner">
                            <Icon icon="eos-icons:loading" width="28" height="28" />
                            <span>Submitting...</span>
                        </div>
                    ) : !submitted ? (
                        <button onClick={handleSubmit}>Submit Election Creation</button>
                    ) : null}
                </div>
            </div>
        </div>
    );
};

export default CreateElectionModal;
