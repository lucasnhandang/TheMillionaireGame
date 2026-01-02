#!/bin/bash

echo "========================================"
echo "   KIEM TRA USER TRONG DATABASE"
echo "========================================"
echo ""

# Check if user exists
echo "1. Checking users in database..."
sudo -u postgres psql millionaire_game -c "SELECT username, role FROM users;" 2>/dev/null

echo ""
echo "2. Checking if user 'hayday' exists..."
COUNT=$(sudo -u postgres psql millionaire_game -t -c "SELECT COUNT(*) FROM users WHERE username = 'hayday';" 2>/dev/null | tr -d ' ')

if [ "$COUNT" = "1" ]; then
    echo "   ✓ User 'hayday' exists"
else
    echo "   ✗ User 'hayday' does NOT exist"
    echo ""
    echo "   Creating user 'hayday'..."
    sudo -u postgres psql millionaire_game << EOF
INSERT INTO users (username, password_hash) 
VALUES ('hayday', 'Hayday2004')
ON CONFLICT (username) DO NOTHING;
EOF
    echo "   ✓ User created (password not hashed - for testing only)"
fi

echo ""
echo "3. All users:"
sudo -u postgres psql millionaire_game -c "SELECT id, username, role FROM users;" 2>/dev/null

echo ""
echo "========================================"
echo ""
echo "NOTE: Current authentication only checks if user exists."
echo "Password hashing is not implemented yet."
echo ""

