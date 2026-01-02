#!/bin/bash

echo "========================================"
echo "   QUICK FIX DATABASE AUTHENTICATION"
echo "========================================"
echo ""

# Find pg_hba.conf
PG_HBA=$(sudo find /etc/postgresql -name "pg_hba.conf" 2>/dev/null | head -1)

if [ -z "$PG_HBA" ]; then
    echo "ERROR: Cannot find pg_hba.conf"
    echo "Try: sudo -u postgres psql -c 'SHOW hba_file;'"
    exit 1
fi

echo "Found pg_hba.conf: $PG_HBA"
echo ""

# Backup
echo "Creating backup..."
sudo cp "$PG_HBA" "${PG_HBA}.backup.$(date +%Y%m%d_%H%M%S)"
echo "✓ Backup created"
echo ""

# Show current settings
echo "Current settings:"
sudo grep -E "^local.*all|^host.*127.0.0.1|^host.*::1" "$PG_HBA" | head -5
echo ""

# Fix all authentication methods to trust
echo "Fixing authentication to 'trust'..."
sudo sed -i 's/^\(local[[:space:]]*all[[:space:]]*all[[:space:]]*\).*$/\1trust/' "$PG_HBA"
sudo sed -i 's/^\(host[[:space:]]*all[[:space:]]*all[[:space:]]*127.0.0.1\/32[[:space:]]*\).*$/\1trust/' "$PG_HBA"
sudo sed -i 's/^\(host[[:space:]]*all[[:space:]]*all[[:space:]]*::1\/128[[:space:]]*\).*$/\1trust/' "$PG_HBA"

echo "✓ Modified"
echo ""

# Show new settings
echo "New settings:"
sudo grep -E "^local.*all|^host.*127.0.0.1|^host.*::1" "$PG_HBA" | head -5
echo ""

# Reload PostgreSQL config
echo "Reloading PostgreSQL configuration..."
sudo systemctl reload postgresql
sleep 2

# Test connection
echo "Testing connection..."
if psql -U postgres -d millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
    echo "✓ Connection successful!"
    echo ""
    echo "Database is ready. Restart server:"
    echo "  cd server && ./bin/server"
else
    echo "✗ Connection still failed"
    echo ""
    echo "Trying full restart..."
    sudo systemctl restart postgresql
    sleep 3
    
    if psql -U postgres -d millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
        echo "✓ Connection successful after restart!"
    else
        echo "✗ Still failed. Check:"
        echo "  1. PostgreSQL is running: sudo systemctl status postgresql"
        echo "  2. Database exists: sudo -u postgres psql -l | grep millionaire_game"
        echo "  3. Check logs: sudo journalctl -u postgresql -n 20"
    fi
fi

echo ""
echo "========================================"

