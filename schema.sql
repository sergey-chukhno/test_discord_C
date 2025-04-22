-- Drop tables if they exist (in reverse order of dependencies)
DROP TABLE IF EXISTS DIRECT_MESSAGES;
DROP TABLE IF EXISTS CHANNEL_MEMBERS;
DROP TABLE IF EXISTS USER_ROLES;
DROP TABLE IF EXISTS ROLES;
DROP TABLE IF EXISTS ATTACHMENTS;
DROP TABLE IF EXISTS REACTIONS;
DROP TABLE IF EXISTS MESSAGES;
DROP TABLE IF EXISTS CHANNELS;
DROP TABLE IF EXISTS USERS;

-- Create USERS table
CREATE TABLE USERS (
    user_id SERIAL PRIMARY KEY,
    first_name VARCHAR(100),
    last_name VARCHAR(100),
    email VARCHAR(150) UNIQUE,
    password_hash VARCHAR(255),
    status VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);

-- Create CHANNELS table
CREATE TABLE CHANNELS (
    channel_id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    description VARCHAR(255),
    channel_type VARCHAR(50),
    created_by INT REFERENCES USERS(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_private BOOLEAN DEFAULT false
);

-- Create MESSAGES table
CREATE TABLE MESSAGES (
    message_id SERIAL PRIMARY KEY,
    user_id INT REFERENCES USERS(user_id),
    channel_id INT REFERENCES CHANNELS(channel_id),
    content TEXT,
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_edited BOOLEAN DEFAULT false,
    is_encrypted BOOLEAN DEFAULT false,
    edited_at TIMESTAMP
);

-- Create REACTIONS table
CREATE TABLE REACTIONS (
    reaction_id SERIAL PRIMARY KEY,
    message_id INT REFERENCES MESSAGES(message_id),
    user_id INT REFERENCES USERS(user_id),
    emoji VARCHAR(50),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create ATTACHMENTS table
CREATE TABLE ATTACHMENTS (
    attachment_id SERIAL PRIMARY KEY,
    message_id INT REFERENCES MESSAGES(message_id),
    file_name VARCHAR(255),
    file_path VARCHAR(255),
    file_size INT,
    mime_type VARCHAR(100),
    uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create ROLES table
CREATE TABLE ROLES (
    role_id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    color VARCHAR(50),
    permission_level INT
);

-- Create USER_ROLES table
CREATE TABLE USER_ROLES (
    user_role_id SERIAL PRIMARY KEY,
    user_id INT REFERENCES USERS(user_id),
    role_id INT REFERENCES ROLES(role_id),
    channel_id INT REFERENCES CHANNELS(channel_id),
    assigned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create CHANNEL_MEMBERS table
CREATE TABLE CHANNEL_MEMBERS (
    member_id SERIAL PRIMARY KEY,
    user_id INT REFERENCES USERS(user_id),
    channel_id INT REFERENCES CHANNELS(channel_id),
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    notifications_enabled BOOLEAN DEFAULT true
);

-- Create DIRECT_MESSAGES table
CREATE TABLE DIRECT_MESSAGES (
    dm_id SERIAL PRIMARY KEY,
    sender_id INT REFERENCES USERS(user_id),
    recipient_id INT REFERENCES USERS(user_id),
    content TEXT,
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_read BOOLEAN DEFAULT false,
    is_encrypted BOOLEAN DEFAULT false
);

-- Create indexes for better performance
CREATE INDEX idx_users_email ON USERS(email);
CREATE INDEX idx_messages_channel_id ON MESSAGES(channel_id);
CREATE INDEX idx_messages_user_id ON MESSAGES(user_id);
CREATE INDEX idx_channel_members_user_id ON CHANNEL_MEMBERS(user_id);
CREATE INDEX idx_channel_members_channel_id ON CHANNEL_MEMBERS(channel_id);
CREATE INDEX idx_direct_messages_sender_id ON DIRECT_MESSAGES(sender_id);
CREATE INDEX idx_direct_messages_recipient_id ON DIRECT_MESSAGES(recipient_id); 