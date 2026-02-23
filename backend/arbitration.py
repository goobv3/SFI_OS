import yaml
import os

def load_rules(filepath="config/interlock_rules.yaml"):
    if not os.path.exists(filepath):
        print(f"Warning: Rule file {filepath} not found.")
        return {}
    try:
        with open(filepath, 'r') as file:
            return yaml.safe_load(file)
    except Exception as e:
        print(f"Error loading rules: {e}")
        return {}

def check_interlock_rules(rules, actuator_type, command, connection):
    """
    Evaluates requested command against defined YAML interlock rules.
    This uses a simplified matcher that bridges the string condition 
    to database status checks.
    """
    if not rules or not connection:
        return {'blocked': False}
        
    interlocks = rules.get('interlocks', [])
    
    with connection.cursor() as cursor:
        for rule in interlocks:
            condition = rule.get('condition', '')
            
            # Rule 1: Heater vs Cooler
            if "target.type == 'HEATER' and command == 'ON'" in condition:
                if actuator_type == 'HEATER' and command == 'ON':
                    cursor.execute("""
                        SELECT status FROM actuator_status s 
                        JOIN actuator_metadata m ON s.actuator_id = m.actuator_id 
                        WHERE m.type = 'COOLER'
                    """)
                    coolers = cursor.fetchall()
                    if any(c.get('status') == 'ON' for c in coolers):
                        return {'blocked': True, 'reason': rule['reason']}

            # Rule 2: Cooler vs Heater
            if "target.type == 'COOLER' and command == 'ON'" in condition:
                if actuator_type == 'COOLER' and command == 'ON':
                    cursor.execute("""
                        SELECT status FROM actuator_status s 
                        JOIN actuator_metadata m ON s.actuator_id = m.actuator_id 
                        WHERE m.type = 'HEATER'
                    """)
                    heaters = cursor.fetchall()
                    if any(h.get('status') == 'ON' for h in heaters):
                        return {'blocked': True, 'reason': rule['reason']}

    return {'blocked': False}
