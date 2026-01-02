#!/bin/bash

echo "========================================"
echo "   TAO USER TRONG DATABASE"
echo "========================================"
echo ""

# Check if user exists
USERNAME="hayday"
PASSWORD="Hayday2004"

echo "Checking if user '$USERNAME' exists..."
EXISTS=$(sudo -u postgres psql millionaire_game -t -c "SELECT COUNT(*) FROM users WHERE username = '$USERNAME';" 2>/dev/null | tr -d ' ')

if [ "$EXISTS" = "1" ]; then
    echo "✓ User '$USERNAME' already exists"
else
    echo "✗ User '$USERNAME' does NOT exist"
    echo ""
    echo "Creating user '$USERNAME'..."
    
    sudo -u postgres psql millionaire_game << EOF
INSERT INTO users (username, password_hash) 
VALUES ('$USERNAME', '$PASSWORD')
ON CONFLICT (username) DO NOTHING;
SELECT 'User created successfully' as result;
EOF
    
    if [ $? -eq 0 ]; then
        echo "✓ User '$USERNAME' created successfully"
    else
        echo "✗ Failed to create user"
        exit 1
    fi
fi

echo ""
echo "User details:"
sudo -u postgres psql millionaire_game -c "SELECT id, username, role, created_at FROM users WHERE username = '$USERNAME';" 2>/dev/null

echo ""
echo "========================================"
echo ""
echo "NOTE: Password is stored as plain text (for testing only)."
echo "In production, password should be hashed."
echo ""
echo "You can now login with:"
echo "  Username: $USERNAME"
echo "  Password: $PASSWORD"
echo ""

