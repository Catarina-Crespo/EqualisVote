import React, { useState } from 'react';
import { Icon } from "@iconify/react";
import { loadWasmFunctions, requestJoinGroup } from '../../backend/wasmAPI';
import { useUser } from '../../context/UserContext';

const JoinGroupModal = ({ isOpen, onClose, onSubmit }) => {
    const { keys } = useUser();
    
    const [spk, setSpk] = useState('');
    const [loading, setLoading] = useState(false);
    const [submitted, setSubmitted] = useState(false);

    if (!isOpen) return null;

    

    const handleSubmit = async () => {
        if (!spk.trim()) {
            alert("Please enter at least one user");
            return;
        }

        setLoading(true);

        console.log()
        const wasm = await loadWasmFunctions();
        await requestJoinGroup(keys.upk, keys.v_i, spk, wasm);

        // simulate request taking 2s
        setTimeout(() => {
            setLoading(false);
            setSubmitted(true);

            // reset after 2s, then close modal
            setTimeout(() => {
                setSpk('');
                setSubmitted(false);
                onClose();
            }, 4000);
        }, 2000);
    };

    return (
        <div className="modal-overlay">
            <div className="modal">
                <div className="modal-header">
                    <h2>Join Group</h2>
                    <button onClick={onClose}>
                        <Icon icon="radix-icons:cross-2" width="24" height="24" />
                    </button>
                </div>

                {!submitted ? (
                    <label>
                        Insert the ID of the group you want to join:
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
                ) : (
                    <div className="encrypted-message">
                        <p><Icon icon="line-md:circle-to-confirm-circle-transition" width="24" height="24" className="success-icon" /> <span>Your join request was sent to the server! Your reputation among the group members is being calculated and soon, you'll receive a notification announcing the result.</span></p>
                    </div>

                )}

                <div className="modal-actions">
                    {loading ? (
                        <div className="loading-spinner">
                            <Icon icon="eos-icons:loading" width="28" height="28" />
                            <span>Submitting...</span>
                        </div>
                    ) : !submitted ? (
                        <button onClick={handleSubmit}>Submit Join Request</button>
                    ) : null}
                </div>
            </div>
        </div>
    );
};

export default JoinGroupModal;
