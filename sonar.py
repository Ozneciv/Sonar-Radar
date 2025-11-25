import serial
import serial.tools.list_ports
import time
import sys

def find_arduino():
    """Tenta encontrar automaticamente a porta do Arduino"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'Arduino' in port.description or 'CH340' in port.description:
            return port.device
    return None

def main():
    port = find_arduino() or 'COM5'  # Fallback para COM5 se não encontrar
    
    try:
        print(f"Tentando conectar à porta {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Aguarda inicialização
        print("Conexão estabelecida com sucesso!")
        
        # Seu código de plotagem aqui...
        
    except serial.SerialException as e:
        print(f"Falha na conexão: {e}")
        print("Verifique:")
        print("1. Se o Arduino está conectado")
        print("2. Se a porta está correta")
        print("3. Se nenhum outro programa está usando a porta")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nPrograma encerrado pelo usuário")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Porta serial fechada")

if __name__ == "__main__":
    main()