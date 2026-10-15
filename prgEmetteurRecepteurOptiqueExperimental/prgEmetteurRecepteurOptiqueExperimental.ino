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
//  Desc Projet :       Programme permettant de faire communiquer 2 cartes ATtiny en UART,
//                      via de la fibre optique (ou plus précisément deux, l'un pour faire
//                      circuler les données dans un sens, et l'autre dans l'autre sens)
//
//  Fichier     :       prgEmetteurRecepteurOptiqueExperimental.ino
//  Créé le     :       27.08.2026
//  Auteur      :       Jérôme TOMSKI
//  Site        :       https://passionelectronique.fr/
//  GitHub      :       https://github.com/PassionElectronique/Emetteur-Recepteur-Optique-Experimental
//  Licence     :       https://creativecommons.org/licenses/by-nc-nd/4.0/deed.fr (BY-NC-ND 4.0 CC)
//
// ==============================================================================================================
//
//  Prérequis   :       - Avoir installé "megatinycore" dans Arduino IDE
//                      - Impérativement sélectionner "1 MHz INTERNAL" dans le menu Outils > Clock
//
//  Remarques   :       - les leds LED1 et LED2 s'allument à l'état haut
//                      - les boutons poussoirs BP1 et BP2 sont actifs à l'état bas
//                        (il faudra activer les pull-up ATtiny ici)
//                      - la liaison UART sera de type : 8 bits, sans parité, et 1 bit de stop
//                        (avec une vitesse de 9600 bauds, par défaut, pour commencer !)
//
// ==============================================================================================================

// -----------------------------------------------------------------------
// Affectation des broches (microcontrôleur ATtiny414, en boitier SOIC-14)
// -----------------------------------------------------------------------
constexpr uint8_t BROCHE_LED_1              = PIN_PA4;
constexpr uint8_t BROCHE_LED_2              = PIN_PA6;
constexpr uint8_t BROCHE_BOUTON_POUSSOIR_1  = PIN_PA5;
constexpr uint8_t BROCHE_BOUTON_POUSSOIR_2  = PIN_PA7;
// + PB2 : TX UART (émission optique)
// + PB3 : RX UART (réception optique)

// ----------
// Constantes
// ----------
constexpr uint32_t VITESSE_TRANSMISSION_UART_EN_BAUDS = 1200;   // Minimum 62 bauds / Maximum 125000 ... mais maximum relativement limité en pratique (en ajustant RV2, j'ai atteint 3900 bauds sur une carte,
                                                                // et 5000 bauds sur une autre). Ici, avec un tel montage artisanal, rien qu'une fibre optique insérée différemment peut influer sur le débit !
constexpr uint8_t  OCTET_DE_SYNCHRONISATION           = 0x5A;   // Bits de synchronisation = 0101 1010 (choisi "au hasard", comme motif de départ de communication UART)
constexpr uint8_t  OCTET_SIGNIFIANT_BP1_APPUYE        = 0x01;   // Valeur signifiant "un appui sur le bouton poussoir #1 a été fait"
constexpr uint8_t  OCTET_SIGNIFIANT_BP2_APPUYE        = 0x02;   // Valeur signifiant "un appui sur le bouton poussoir #2 a été fait"
constexpr uint16_t DELAI_REARMEMENT_BP_EN_MS          = 30;     // 30 ms de délai "anti-rebond logiciel", pour chaque bouton poussoir

// ---------------------------------------------------------------------------------------------
// Structure définissant "un bouton poussoir", avec les propriétés qu'on décide de lui adjoindre
// ---------------------------------------------------------------------------------------------
struct BoutonPoussoir {
  uint8_t  broche_attiny_utilisee;            // Permet de dire quelle broche ATtiny sera affectée au bouton-poussoir en question
  bool     precedent_etat_lu;                 // Permet de mémoriser le précédent état lu du bouton poussoir (en sachant que ce programme va scanner l'état des BPs à chaque passage en boucle loop)
  uint32_t timing_dernier_changement_d_etat;  // Permet de mémoriser le timing arduino (compteur temps qui a démarré à 0 à la mise sous tension ou reset de l'arduino) au moment du dernier changement d'état du BP
  bool     bouton_precedemment_relache;       // Permet de dire si le bouton était précédemment relaché ou non (avec anti-rebond post-relachement)
};

// ---------------------------------------------------------------------------------------------
// Déclaration de nos 2 boutons poussoirs, à l'aide de la structure 'struct' précédemment créée)
// ---------------------------------------------------------------------------------------------
BoutonPoussoir bp1{BROCHE_BOUTON_POUSSOIR_1, HIGH, 0, true};  // HIGH, O, false permet donc de définir initialement : precedent_etat_lu = HIGH, timing_dernier_changement_d_etat = 0,
BoutonPoussoir bp2{BROCHE_BOUTON_POUSSOIR_2, HIGH, 0, true};  //                                                      et bouton_precedemment_relache = true


// ========================
// Initialisation programme
// ========================
void setup() {

  // Configuration des broches ATtiny où seront branchées les LEDs
  pinMode(BROCHE_LED_1, OUTPUT);
  pinMode(BROCHE_LED_2, OUTPUT);      // Broches configurées en "sortie"
  digitalWrite(BROCHE_LED_1, LOW);    // et mises à l'état bas, pour commencer (leds éteintes)
  digitalWrite(BROCHE_LED_2, LOW);

  // Configuration des broches ATtiny où seront branchés les BOUTONS POUSSOIRS
  pinMode(BROCHE_BOUTON_POUSSOIR_1, INPUT_PULLUP);
  pinMode(BROCHE_BOUTON_POUSSOIR_2, INPUT_PULLUP);    // Broches configurées en "entrée", et pull-ups ATtiny activées

  // Configuration de la liaison UART
  Serial.swap(0);     // Nota : le "0" permet de sélectionner les broches UART par défaut (si j'avais mis 1, il aurait fallu utiliser les broches alternatives TX/RX de l'ATtiny, mais ce n'est pas le cas ici)
  Serial.begin(VITESSE_TRANSMISSION_UART_EN_BAUDS, SERIAL_8N1);   // Initialisation de la liaison UART, à la vitesse choisie plus haut (en mode "8N1", donc : 8 bits / sans parité / 1 bit de stop)

}


// =================
// Boucle principale
// =================
void loop() {

  // ----------------------
  // Partie "émission UART"
  // ----------------------
  
  // On check l'état du 1er bouton-poussoir, et s'il est appuyé, alors on envoie un message via l'UART signifiant cela
  if (boutonAppuye(bp1)) envoieOctet(OCTET_SIGNIFIANT_BP1_APPUYE);

  // On check l'état du 2nd bouton-poussoir (et s'il est appuyé, on envoie un message via l'UART signifiant cela)
  if (boutonAppuye(bp2)) envoieOctet(OCTET_SIGNIFIANT_BP2_APPUYE);

  // -----------------------
  // Partie "réception UART"
  // -----------------------

  // On check si des données ont été reçues via l'UART (et sont donc en attente de lecture)
  while (Serial.available() > 0) {
    // Si c'est le cas, alors on lit un octet uniquement, on le traite, puis on passe aux éventuels suivants
    uint8_t octetRecu = Serial.read();
    traiteOctetRecu(octetRecu);
  }
  
}


// ======================
// Fonction "envoieOctet"
// ======================
void envoieOctet(uint8_t octet_a_envoyer) {

  // Afin que le récepteur soit "sûr" que l'octet qu'il reçoit est bon, on va "encadrer" cet "octet à envoyer" entre un octet de synchronisation (motif 8 bits toujours identique, signifiant le
  // début d'une transmission), et le complément de cet octet (tous les bits inversés, donc, afin de pouvoir faire une comparaison simple ensuite, vérifiant l'intégrité ou non de la donnée transmise).

  uint8_t complement_de_l_octet_a_envoyer = (~octet_a_envoyer);
  
  // Du coup, la trame (suite de 3 octets ici) qui sera envoyée via l'UART se décomposera ainsi :
  //    1) un octet de synchronisation
  //    2) l'octet à envoyer
  //    3) et le complément de cet octet à envoyer

  Serial.write(OCTET_DE_SYNCHRONISATION);
  Serial.write(octet_a_envoyer);
  Serial.write(complement_de_l_octet_a_envoyer);
  
}


// =======================
// Fonction "boutonAppuye"
// =======================
bool boutonAppuye(BoutonPoussoir &bp) {

  // Lit l'état actuel du bouton poussoir visé
  const bool etat_actuel_bp = digitalRead(bp.broche_attiny_utilisee);

  // Récupération du compteur temps arduino, pour pouvoir ensuite comparer cela au timing mémorisé dans la "struct" de ce bouton-poussoir
  const uint32_t maintenant = millis();

  // Si l'état a changé depuis la dernière lecture du BP, alors on mémorise ce nouvel état, et la valeur du compteur-temps arduino à cet instant
  if (etat_actuel_bp != bp.precedent_etat_lu) {
    bp.precedent_etat_lu = etat_actuel_bp;
    bp.timing_dernier_changement_d_etat = maintenant;
  }

  // Si ce bouton vient juste d'être appuyé, alors on retourne "true" pour que la transmission UART démarre
  if (etat_actuel_bp == LOW && bp.bouton_precedemment_relache) {
    bp.bouton_precedemment_relache = false;
    return true;
  }

  // Si ce bouton a été relaché, on regarde depuis combien de temps, et on réinitialise son état si le délai de réarmement a été dépassé
  if (etat_actuel_bp == HIGH && !bp.bouton_precedemment_relache) {
    if (maintenant - bp.timing_dernier_changement_d_etat >= DELAI_REARMEMENT_BP_EN_MS) {
      bp.bouton_precedemment_relache = true;
    }
  }

  // Dans tous les autres cas, on retourne false
  return false;
  
}


// ==========================
// Fonction "traiteOctetRecu"
// ==========================
void traiteOctetRecu(uint8_t octet_recu) {

  // Liste énumérant les différents états que peut prendre RX, dans ce programme
  enum ListeDesEtatsPourRX : uint8_t { ATTEND_OCTET_DE_SYNCHRONISATION, ATTEND_OCTET_DE_DONNEE, ATTEND_COMPLEMENT_OCTET_DE_DONNEE };

  // Variable enregistrant l'état courant de RX ("static" permet de mémoriser cette variable après sortie de cette fonction, pour le "tour d'après")
  static ListeDesEtatsPourRX etatDeRX = ATTEND_OCTET_DE_SYNCHRONISATION;

  // Variable permettant de mémoriser la donnée qui sera reçue (pour la comparer ensuite à son complément, qu'on devrait recevoir en suivant)
  static uint8_t donnee_transmise;

  // Actions à faire, selon les différents états possibles de RX
  switch (etatDeRX) {

    // ---------------------------------------
    // Cas : "ATTEND_OCTET_DE_SYNCHRONISATION"
    // ---------------------------------------
    case ATTEND_OCTET_DE_SYNCHRONISATION:
      if (octet_recu == OCTET_DE_SYNCHRONISATION) {
        // Si on a bien reçu un octet de synchronisation (un "motif" permettant de repérer le début d'une trame UART, ici), alors on passe à la suite (attente de "l'octet de donnée")
        etatDeRX = ATTEND_OCTET_DE_DONNEE;
      }
      break;

    // ------------------------------
    // Cas : "ATTEND_OCTET_DE_DONNEE"
    // ------------------------------
    case ATTEND_OCTET_DE_DONNEE:
      // Si on a bien reçu un octet signifiant un appui sur BP1 ou BP2, alors on mémorise cet état, et on passe à la suite (attente du "complément")
      if (octet_recu == OCTET_SIGNIFIANT_BP1_APPUYE || octet_recu == OCTET_SIGNIFIANT_BP2_APPUYE) {
        donnee_transmise = octet_recu;
        etatDeRX = ATTEND_COMPLEMENT_OCTET_DE_DONNEE;
      } else {
        // Sinon, on revient "à la case départ", en attendant une nouvelle trame (commençant par "l'octet de synchronisation")
        etatDeRX = ATTEND_OCTET_DE_SYNCHRONISATION;
      }
      break;

    // ------------------------------
    // Cas : "ATTEND_OCTET_DE_DONNEE"
    // ------------------------------
    case ATTEND_COMPLEMENT_OCTET_DE_DONNEE:
      // Calcul rapide du complément, à partir de la précédente donnée transmise mémorisée
      uint8_t complement_donnee_transmise = ~donnee_transmise;
      // Si l'octet reçu correspond à ce complément, alors on agit sur les LEDs en fonction de quel BP a été appuyé
      if (octet_recu == complement_donnee_transmise) {
        if (donnee_transmise == OCTET_SIGNIFIANT_BP1_APPUYE) {          // Nota : "octet_recu" contient le complément de "donnee_transmise" ; il faut donc réutiliser
          digitalWrite(BROCHE_LED_1, !digitalRead(BROCHE_LED_1));       //        "donnee_transmise" non complémenté ici, pour faire les tests de correspondance
        } else if (donnee_transmise == OCTET_SIGNIFIANT_BP2_APPUYE) {
          digitalWrite(BROCHE_LED_2, !digitalRead(BROCHE_LED_2));
        }
      }
      // Et dans tous les cas, que l'octet ait bien été un complément ou qu'il y ait eu une erreur, on retourne "à la case départ" (à savoir : attente d'un nouvel "octet de synchronisation")
      etatDeRX = ATTEND_OCTET_DE_SYNCHRONISATION;
      break;
      
  }
  
}
