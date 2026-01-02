#!/bin/bash

echo "========================================"
echo "   FIX POSTGRESQL AUTHENTICATION"
echo "========================================"
echo ""

# Find pg_hba.conf
echo "1. Finding pg_hba.conf..."
PG_HBA=$(sudo find /etc/postgresql -name "pg_hba.conf" 2>/dev/null | head -1)

if [ -z "$PG_HBA" ]; then
    echo "   ✗ Cannot find pg_hba.conf"
    echo "   Try: sudo -u postgres psql -c 'SHOW hba_file;'"
    exit 1
fi

echo "   ✓ Found: $PG_HBA"
echo ""

# Backup
echo "2. Creating backup..."
sudo cp "$PG_HBA" "${PG_HBA}.backup.$(date +%Y%m%d_%H%M%S)"
echo "   ✓ Backup created"
echo ""

# Check current settings
echo "3. Current authentication settings:"
sudo grep -E "^local|^host.*127.0.0.1|^host.*::1" "$PG_HBA" | head -3
echo ""

# Ask for confirmation
read -p "Change to 'trust' authentication? (y/n): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 0
fi

# Modify pg_hba.conf
echo "4. Modifying pg_hba.conf..."
sudo sed -i 's/^local.*all.*all.*peer$/local   all             all                                     trust/' "$PG_HBA"
sudo sed -i 's/^host.*all.*all.*127.0.0.1\/32.*md5$/host    all             all             127.0.0.1\/32            trust/' "$PG_HBA"
sudo sed -i 's/^host.*all.*all.*::1\/128.*md5$/host    all             all             ::1\/128                 trust/' "$PG_HBA"

echo "   ✓ Modified"
echo ""

# Restart PostgreSQL
echo "5. Restarting PostgreSQL..."
sudo systemctl restart postgresql
if [ $? -eq 0 ]; then
    echo "   ✓ PostgreSQL restarted"
else
    echo "   ✗ Failed to restart PostgreSQL"
    exit 1
fi
echo ""

# Test connection
echo "6. Testing connection..."
if psql -U postgres -d millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
    echo "   ✓ Connection successful!"
else
    echo "   ✗ Connection failed"
    echo "   Check PostgreSQL logs: sudo journalctl -u postgresql -n 50"
    exit 1
fi
echo ""

echo "========================================"
echo "   HOAN TAT!"
echo "========================================"
echo ""
echo "PostgreSQL da duoc cau hinh de khong can password."
echo "Restart server: cd server && ./bin/server"
echo ""

