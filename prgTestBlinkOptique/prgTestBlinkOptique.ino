// ===================================================================================================
//   ______               _                  _///_ _           _                   _
//  /   _  \             (_)                |  ___| |         | |                 (_)
//  |  [_|  |__  ___  ___ _  ___  _ __      | |__ | | ___  ___| |_ _ __ ___  _ __  _  ___  _   _  ___
//  |   ___/ _ \| __|| __| |/ _ \| '_ \_____|  __|| |/ _ \/  _|  _| '__/   \| '_ \| |/   \| | | |/ _ \
//  |  |  | ( ) |__ ||__ | | ( ) | | | |____| |__ | |  __/| (_| |_| | | (_) | | | | | (_) | |_| |  __/
//  \__|   \__,_|___||___|_|\___/|_| [_|    \____/|_|\___|\____\__\_|  \___/|_| |_|_|\__  |\__,_|\___|
//                                                                                      | |
//                                                                                      \_|
// ===================================================================================================
//
//  Nom Projet  :       Émetteur/Récepteur Optique Expérimental (ATtiny, UART)
//
//  Desc Projet :       Programme permettant de tester la partie optique, via un simple "blink" sur la
//                      led émettrice vers la fibre optique, et allumage des LEDS embarquées (LED1/LED2)
//                      chaque fois qu'une lumière est reçue de l'autre côté de la fibre, côté récepteur
//
//  Fichier     :       prgTestBlinkOptique.ino
//  Créé le     :       31.08.2026
//  Auteur      :       Jérôme TOMSKI
//  Site        :       https://passionelectronique.fr/
//  GitHub      :       https://github.com/PassionElectronique/Emetteur-Recepteur-Optique-Experimental
//  Licence     :       https://creativecommons.org/licenses/by-nc-nd/4.0/deed.fr (BY-NC-ND 4.0 CC)
//
// ==============================================================================================================
//
//  Prérequis   :       Avoir installé "megatinycore" dans Arduino IDE
//
//  Remarque    :       Le UART étant actif à l'état bas, et au repos à l’état haut, la logique est donc inversée
//                      au niveau de la LED d'émission, ainsi qu'au niveau du phototransistor de réception
//
// ==============================================================================================================

// -----------------------------------------------------------------------
// Affectation des broches (microcontrôleur ATtiny414, en boitier SOIC-14)
// -----------------------------------------------------------------------
constexpr uint8_t BROCHE_LED_1                = PIN_PA4;
constexpr uint8_t BROCHE_LED_2                = PIN_PA6;
constexpr uint8_t BROCHE_EMISSION_OPTIQUE     = PIN_PB2;
constexpr uint8_t BROCHE_RECEPTION_OPTIQUE    = PIN_PB3;

// ----------
// Constantes
// ----------
constexpr uint32_t INTERVALLE_DE_BLINK_EN_MS  = 1000;     // Le blink fera donc des alternances "1 seconde allumé" puis "1 seconde éteint", répété à l'infini


// ========================
// Initialisation programme
// ========================
void setup() {

  // ---------------------------------------------------------------------------------------------
  // Configuration des sorties ATtiny (fixation de l'état pour commencer, puis activation ensuite)
  // ---------------------------------------------------------------------------------------------
  digitalWrite(BROCHE_LED_1, LOW);              // LED1/LED2 s'éteignent en les mettant à l'état bas
  digitalWrite(BROCHE_LED_2, LOW);
  digitalWrite(BROCHE_EMISSION_OPTIQUE, HIGH);  // ... tandis que la LED émettrice optique suit une logique inversée (il faut donc mettre cette sortie à l'état haut, pour éteindre cette led)

  pinMode(BROCHE_LED_1, OUTPUT);
  pinMode(BROCHE_LED_2, OUTPUT);
  pinMode(BROCHE_EMISSION_OPTIQUE, OUTPUT);

  // --------------------------------
  // Configuration des entrées ATtiny  
  // --------------------------------
  pinMode(BROCHE_RECEPTION_OPTIQUE, INPUT);

  // Petit délai, avant de passer à la boucle loop
  delay(200);

}


// =================
// Boucle principale
// =================
void loop() {
  
  // Variables locales
  static bool etatBlink = HIGH;                           // Mémorisation ("static") de l'état de blink, qu'on va alterner ON/OFF (low/high) à intervalle régulier (mise à "HIGH", pour commencer)
  static uint32_t timingDernierChangementEtatBlink = 0;   // Mémorisation ("static") du compteur-temps arduino lors du dernier changement d'état de blink (mise à "0" pour commencer)
  const uint32_t maintenant = millis();                   // Récupération de la valeur du compteur-temps arduino actuel, à chaque passage dans cette boucle loop


  // -------------------------
  // Partie "émission optique"
  // -------------------------
  if (maintenant - timingDernierChangementEtatBlink >= INTERVALLE_DE_BLINK_EN_MS) {
    // S'il s'est passé un temps au moins égal à "INTERVALLE_DE_BLINK_EN_MS" depuis la dernière initialisation de "timingDernierChangementEtatBlink", alors :
    //    1) on change l'état de notre variable "etatBlink" (ce qui devrait arriver dès le premier passage, dû au fait qu'on a mis 'delay=200' dans la fonction setup)
    //    2) on enregistre le compteur-temps arduino au moment de cet évènement
    //    3) on pilote la led émettrice optique en conséquence
    
    etatBlink = !etatBlink;
    timingDernierChangementEtatBlink = maintenant;
    digitalWrite(BROCHE_EMISSION_OPTIQUE, etatBlink==HIGH ? LOW : HIGH);    // Logique inversee : la LED émettrice sera allumée si on envoie un état bas, et éteinte si on envoie un état haut
  }                                                                         //                    (logique câblée ainsi, pour l'implémentation du UART plus tard)


  // --------------------------
  // Partie "réception optique"
  // --------------------------

  // On check si un signal lumineux est actuellement reçu (en inversant également la logique ici au passage, car c'est routé ainsi sur le PCB)
  const bool signalOptiqueActuellementRecu = (digitalRead(BROCHE_RECEPTION_OPTIQUE)==LOW);

  // Et on pilote les leds embarquées (LED1 et LED2) en conséquence
  digitalWrite(BROCHE_LED_1, signalOptiqueActuellementRecu==HIGH ? HIGH : LOW);
  digitalWrite(BROCHE_LED_2, signalOptiqueActuellementRecu==HIGH ? HIGH : LOW);

}
