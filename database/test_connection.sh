#!/bin/bash

echo "========================================"
echo "   TEST DATABASE CONNECTION"
echo "========================================"
echo ""

# Test with sudo -u postgres
echo "1. Testing connection with sudo -u postgres..."
if sudo -u postgres psql -d millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
    echo "   ✓ Connection successful!"
    echo ""
    echo "   Database info:"
    sudo -u postgres psql -d millionaire_game -c "SELECT version();" | head -3
    echo ""
    echo "   Questions count:"
    sudo -u postgres psql -d millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"
else
    echo "   ✗ Connection failed"
    echo ""
    echo "   Trying to fix authentication..."
    echo ""
    
    # Find and fix pg_hba.conf
    PG_HBA=$(sudo find /etc/postgresql -name "pg_hba.conf" 2>/dev/null | head -1)
    if [ -n "$PG_HBA" ]; then
        echo "   Found pg_hba.conf: $PG_HBA"
        echo "   Backing up..."
        sudo cp "$PG_HBA" "${PG_HBA}.backup.$(date +%Y%m%d_%H%M%S)"
        
        echo "   Modifying authentication to 'trust'..."
        sudo sed -i 's/^local.*all.*all.*peer$/local   all             all                                     trust/' "$PG_HBA"
        sudo sed -i 's/^host.*all.*all.*127.0.0.1\/32.*md5$/host    all             all             127.0.0.1\/32            trust/' "$PG_HBA"
        sudo sed -i 's/^host.*all.*all.*::1\/128.*md5$/host    all             all             ::1\/128                 trust/' "$PG_HBA"
        
        echo "   Restarting PostgreSQL..."
        sudo systemctl restart postgresql
        
        sleep 2
        
        echo "   Testing again..."
        if psql -U postgres -d millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
            echo "   ✓ Connection successful after fix!"
        else
            echo "   ✗ Still failed. Check PostgreSQL logs:"
            echo "     sudo journalctl -u postgresql -n 20"
        fi
    else
        echo "   ✗ Cannot find pg_hba.conf"
        echo "   Manual fix required. See server/FIX_POSTGRESQL_AUTH.md"
    fi
fi

echo ""
echo "========================================"
echo ""

