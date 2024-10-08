
import pygame
import requests
import time



ESP32_IP = "192.168.204.199"  # Replace with the actual IP address of your ESP32
URLS = {
    "T_entrada": f"http://{ESP32_IP}/T_I",
    "T_salida": f"http://{ESP32_IP}/T_O",
    "T_aire": f"http://{ESP32_IP}/T_A",
    "Humedad": f"http://{ESP32_IP}/H",

    "Ventilador": f"http://{ESP32_IP}/F",
    "Caudal": f"http://{ESP32_IP}/P"
}

POSITIONS = {
    "T_entrada": (1200, 225),
    "T_salida": (400, 650),
    "T_aire": (1000, 350),
    "Humedad": (450, 450),

    "Ventilador": (300, 350),
    "Caudal": (1200, 650)
}

FAN_SPEED = 0
VALVE_SPEED = 0

class Button:
    def __init__(self, label, rect, color):
        self.label = label
        self.rect = pygame.Rect(rect)
        self.color = color
        self.font = pygame.font.Font(None, 30)
        self.text = self.font.render(self.label, True, (255, 255, 255))
        self.text_rect = self.text.get_rect()
        self.text_rect.center = (self.rect.x + self.rect.width // 2, self.rect.y + self.rect.height // 2)

    def draw(self, screen):
        pygame.draw.rect(screen, self.color, self.rect)
        screen.blit(self.text, self.text_rect)


# Fetch data from ESP32
def get_data_from_esp32(url):
    try:
        response = requests.get(url)
        if response.status_code == 200:
            return response.text
        else:
            return f"Error: {response.status_code}"
    except requests.exceptions.RequestException as e:
        return f"Error: {e.response}"


# Pygame setup
pygame.init()
screen_width, screen_height = 1600, 900
screen = pygame.display.set_mode((screen_width, screen_height))
pygame.display.set_caption('ESP32 Data Display')
font = pygame.font.Font(None, 36)
clock = pygame.time.Clock()


inicio = Button("Iniciar", (560, 750, 140, 50), (0, 0, 255))
paro = Button("Detener", (900, 750, 140, 50), (0, 0, 255))

v_d = Button("-", (175, 340, 50, 50), (0, 0, 255))
v_u = Button("+", (375, 340, 50, 50), (0, 0, 255))
p_d = Button("-", (1075, 640, 50, 50), (0, 0, 255))
p_u = Button("+", (1275, 640, 50, 50), (0, 0, 255))

# Periodic data update
last_update_time = 0
update_interval = 1  # seconds
data = {}

data1 = {
    "T_entrada": f"50.5 °C",
    "T_salida": f"35.0 °C",
    "Humedad": f"60%",
    "T_aire": f"30.0 °C",

    "Ventilador": f"80%",
    "Caudal": f"5.86ml/s"
}

# Load the image
image = pygame.image.load('Torre_enfriamiento_2.png')
# Get the rect of the image
image_rect = image.get_rect()
# Calculate the center position for the image
image_rect.center = (screen_width // 2, screen_height // 2 - 100)

fan = pygame.image.load('fan_1.png')
fan_rect = fan.get_rect()
fan_rect.center = (300, 200)

valve = pygame.image.load('valve.png')
valve_rect = valve.get_rect()
valve_rect.center = (1200, 500)

# Initial data fetch
for label, url in URLS.items():
    data[label] = get_data_from_esp32(url)

running = True
while running:
    current_time = time.time()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if inicio.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/I?value={1}")
                print(info)
            elif paro.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/I?value={2}")
                print(info)
            elif v_d.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/F?value={0}")
                print(info)
            elif v_u.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/F?value={1}")
                print(info)
            elif p_d.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/P?value={0}")
                print(info)
            elif p_u.rect.collidepoint(event.pos):
                info = get_data_from_esp32(f"http://{ESP32_IP}/P?value={1}")
                print(info)

    # Update data periodically
    if current_time - last_update_time > update_interval:
        for label, url in URLS.items():
            data[label] = get_data_from_esp32(url)
        last_update_time = current_time

    screen.fill((255, 255, 255))

    screen.blit(image, image_rect)
    screen.blit(fan, fan_rect)
    screen.blit(valve, valve_rect)

    # Display data
    for label, value in data.items():

        display = ""

        if label == "Humedad" or label == "Ventilador":
            display = f"{int(float(value))}%"
        elif label == "Caudal":
            #float ans = min(14.029f * n + 67.702f, 255.0f);
            display = f"{(max(0.0, float(value) * 0.182 - 4.826)):.2f}ml/s"
        else:
            display = f"{value}°C"

        text_surface_label = font.render(f"{label}:", True, (0, 0, 0))
        text_surface_value = font.render(f"{display}", True, (0, 0, 0))

        # Get the rect of the text surface
        text_rect_label = text_surface_label.get_rect()
        text_rect_value = text_surface_value.get_rect()

        # Calculate the center position for the text
        _x, _y = POSITIONS[label]
        text_rect_label.center = _x, _y
        text_rect_value.center = _x, _y + 25

        screen.blit(text_surface_label, text_rect_label)
        screen.blit(text_surface_value, text_rect_value)

    # Draw buttons
    inicio.draw(screen)
    paro.draw(screen)

    v_d.draw(screen)
    v_u.draw(screen)
    p_d.draw(screen)
    p_u.draw(screen)

    pygame.display.flip()
    clock.tick(60)

pygame.quit()

'''
uint8_t percent(float n){
  return (uint8_t)(2.55f* n);
}

uint8_t caudal(float n){
  n = max(0.0f, n);
  float ans = min(14.029f * n + 67.702f, 255.0f);
  return (uint8_t)ans;
}
'''