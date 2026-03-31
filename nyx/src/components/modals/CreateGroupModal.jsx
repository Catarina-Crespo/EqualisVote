import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import { createGroup } from '../../backend/wasmAPI';

const CreateGroupModal = ({ isOpen, onClose, onSubmit }) => {
    const [groupName, setName] = useState('');
    const [members, setMembers] = useState('');
    const [loading, setLoading] = useState(false);
    const [submitted, setSubmitted] = useState(false);

    if (!isOpen) return null;

    const handleSubmit = () => {
        if (!members.trim()) {
            alert("Please enter at least one user");
            return;
        }

        console.log(members);

        // TODO: Verify the format of the upks/usernames (or do it on the server)
        //createGroup(members);

        setLoading(true);

        // simulate request taking 2s
        setTimeout(() => {
            setLoading(false);
            setSubmitted(true);

            // call parent onSubmit with parsed users
            //onSubmit({ members: members.split(",").map(m => m.trim()) });

            const parsedMembers = members.split(",").map(m => m.trim());
            createGroup(groupName, parsedMembers);

            if (onSubmit) {
                setTimeout(() => onSubmit(), 5000)
            }

            // reset after 2s, then close modal
            setTimeout(() => {
                setMembers('');
                setSubmitted(false);
                onClose();
            }, 4000);
        }, 2000);
    };

    return (
        <div className="modal-overlay">
            <div className="modal">
                <div className="modal-header">
                    <h2>Create Group</h2>
                    <button onClick={onClose}>
                        <Icon icon="radix-icons:cross-2" width="24" height="24" />
                    </button>
                </div>

                {!submitted ? (
                    <div>
                        <label>
                            Insert the group name:
                            <input
                                type="text"
                                value={groupName}
                                onChange={(e) => setName(e.target.value)}
                            />
                            <p className="input-help">
                                <Icon
                                    icon="material-symbols:help-rounded"
                                    width="12"
                                    height="12"
                                    className="help-icon"
                                />
                                e.g. Câmara de Almada
                            </p>
                        </label>
                        <label>
                            Insert the username or public ID of the users you want to add to the group (comma-separated):
                            <textarea
                                rows={6}
                                value={members}
                                onChange={(e) => setMembers(e.target.value)}
                            />
                            <p className="input-help">
                                <Icon
                                    icon="material-symbols:help-rounded"
                                    width="12"
                                    height="12"
                                    className="help-icon"
                                />
                                e.g. @john_doe, @jane_doe, 84963f19fc...
                            </p>
                        </label>
                    </div>
                ) : (
                    <div className="encrypted-message">
                        <p><Icon icon="line-md:circle-to-confirm-circle-transition" width="24" height="24" className="success-icon" /> <span>The group creation request was submitted!
                            You'll receive a notification once it's confirmed.</span></p>
                    </div>

                )}

                <div className="modal-actions">
                    {loading ? (
                        <div className="loading-spinner">
                            <Icon icon="eos-icons:loading" width="28" height="28" />
                            <span>Submitting...</span>
                        </div>
                    ) : !submitted ? (
                        <button onClick={handleSubmit}>Submit Group Creation</button>
                    ) : null}
                </div>
            </div>
        </div>
    );
};

export default CreateGroupModal;
