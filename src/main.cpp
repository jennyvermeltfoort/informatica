#include <cstdlib>
#include <ctime>
#include <iostream>

/* Metadata. */
#define STUDENT_NUMBER 3787494
#define STUDENT_NAME "Jenny Vermeltfoort"
#define DATE_UPDATED "09-01-2024"
#define OPDRACHT "1"
#define STUDIE "Bachelor Informatica"

const int MATH_ANSWER_CHEAT = 91823;  // For testing purposes.
const float MATH_ANSWER_FLOAT_LIMIT = 0.1;
const int DATE_NORM_YEAR = 1876;  // Calculations will be normalized to saturday, Januari 1th 1876.
                                  // Oldest person alive.
                                  // 1876 is a schrikkeljaar
const int DAYS_IN_YEAR = 365;
const int LIMIT_FORMAL_AGE = 30;

/* A struct that defines a variable question inlcuding answer asked to the user. */
typedef struct QUESTION_ITEM {
    char question[164];
    char choice_1[128];
    char choice_2[128];
    char choice_3[128];
    char choice_4[128];
    char answer;
} question_item_t;

/* An enumeration that describes the days in a week. */
typedef enum DAY {
    DATE_DAY_MONDAY,
    DATE_DAY_TUESDAY,
    DATE_DAY_WEDNESDAY,
    DATE_DAY_THURSDAY,
    DATE_DAY_FRIDAY,
    DATE_DAY_SATERDAY,
    DATE_DAY_SUNDAY,
} date_day_e;

/* A struct that defines all variable prompts asked to the user. */
typedef struct prompt_list_t {
    char year[164];
    char month[164];
    char day[164];
    char exp_day_str[164];
    char math[164];
    char math_teller[164];
    char math_noemer[164];
    char math_float[164];
    char math_fault[164];
    char question[164];
    char question_fault[164];
    char ok_exact[164];
    char ok_beta[164];
    char done[164];
    char err_invalid[164];
} prompt_list_t;

/* Informal question. */
const question_item_t question_item_informal = {
    .question = "> Welk literair werk wordt beschouwd als het eerste moderne psychologische roman?",
    .choice_1 = "a. Don Quichot van Miguel de Cervantes",
    .choice_2 = "b. De Gebroeders Karamazov van Fjodor Dostojevski",
    .choice_3 = "c. Odyssee van Homerus",
    .choice_4 = "d. De Toverberg van Thomas Mann",
    .answer = 'b'};

/* Formal question. */
const question_item_t question_item_formal = {
    .question =
        "> Welke Franse schrijver en filosoof schreef het werk L'Etranger en won de Nobelprijs "
        "voor Literatuur in 1957?",
    .choice_1 = "a. Jean-Paul Sartre",
    .choice_2 = "b. Albert Camus",
    .choice_3 = "c. Marcel Proust",
    .choice_4 = "d. Andre Gide",
    .answer = 'b'};

/* Informal prompts. */
const prompt_list_t prompt_list_informal = {
    .exp_day_str = "> Welke dag ben je geboren? (m,d,w,v,z)",
    .math = "> Vind het antwoord van het volgende probleem: ",
    .math_teller = "> Geef de teller van je antwoord.",
    .math_noemer = "> Geef de noemer van je antwoord.",
    .math_float = "> Geef het antwoord met twee decimalen na de komma.",
    .math_fault = "> Jammer genoeg is het antwoord niet goed, we gaan verder.",
    .question = "> Geef hier het antwoord op de vraag. (a, b, c, d)",
    .question_fault =
        "> Helaas klopt dat niet helemaal, waarschijnlijk ben je niet geschikt voor een opleiding "
        "aan de universiteit.",
    .ok_exact = "> Helemaal goed, je bent geschikt voor een exacte studie.",
    .ok_beta = "> Helemaal goed, je bent geschikt voor een beta studie.",
    .done = "> Je bent helemaal klaar!",
    .err_invalid = "> Je gegeven informatie klopt niet helemaal, probeer het opnieuw."};

/* Formal prompts. */
const prompt_list_t prompt_list_formal = {
    .year = "> Geef je geboorte jaar.",
    .month = "> Geef de maand waarin je geboren bent.",
    .day = "> Geef de dag waarop je geboren bent.",
    .exp_day_str = "> Verstrek de dag waarop u bent geboren. (m,d,w,v,z)",
    .math = "> Verstrek het antwoord van het volgende probleem: ",
    .math_teller = "> Geef de teller van het antwoord.",
    .math_noemer = "> Geef de noemer van het antwoord.",
    .math_float = "> Geef het antwoord met twee decimalen na de komma.",
    .math_fault =
        "> Helaas is dit fout, mogelijk is een beta studie iets voor u, de vragenlijst gaat door.",
    .question = "> Verstrek het antwoord op de vraag. (a, b, c, d)",
    .question_fault =
        "> Helaas is het antwoord fout, u bent waarschijnlijk niet geschikt voor een opleiding aan "
        "de universiteit.",
    .ok_exact = "> Helemaal goed, u bent geschikt voor een exacte studie.",
    .ok_beta = "> Helemaal goed, u bent geschikt voor een beta studie.",
    .done = "> U heeft de vragenlijst afgerond, een prettige dag gewenst.",
    .err_invalid = "> De gegeven informatie is niet valide, probeer het opnieuw."};

/* Amount of days per month from jan till dec.*/
const int DAY_MONTH_SIZE = 12;
const int day_month[DAY_MONTH_SIZE] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
/* A list of weekdays, normalized to DATE_NORM_YEAR. 1/1/1876 is a saterday.*/
const date_day_e day_list[] = {DATE_DAY_SATERDAY, DATE_DAY_SUNDAY,    DATE_DAY_MONDAY,
                               DATE_DAY_TUESDAY,  DATE_DAY_WEDNESDAY, DATE_DAY_THURSDAY,
                               DATE_DAY_FRIDAY};

int main(int argc, char **argv) {
    time_t time_now = time(NULL);
    struct tm *time_holder = {0};
    time_t time_converted = {0};
    int input_int = 0;
    float input_float = 0;
    char input_char = 0;
    int birth_year = 0;
    int birth_month = 0;
    int birth_day = 0;
    char math_sign = '-';
    int math_1_teller = 0;
    int math_1_noemer = 0;
    int math_2_teller = 0;
    int math_2_noemer = 0;
    int math_answer_teller = 0;
    int math_answer_noemer = 0;
    float math_answer_float = 0;
    int num_leap_days = 0;
    int num_days_total = 0;
    date_day_e exp_day_str = DATE_DAY_MONDAY;
    const prompt_list_t *prompt_list = &prompt_list_formal;
    const question_item_t *question_item = &question_item_formal;

    time_holder = localtime(&time_now);
    srand(time(NULL));

    // Infoblock
    std::cout << "Naam:\t\t\t" << STUDENT_NAME << "\n";
    std::cout << "Student nummer:\t\t" << STUDENT_NUMBER << "\n";
    std::cout << "Laatst geupdate:\t" << DATE_UPDATED << "\n";
    std::cout << "Opdracht:\t\t" << OPDRACHT << "\n";
    std::cout << "Studie:\t\t\t" << STUDIE << "\n\n";
    std::cout << "> Vragenlijst: Ben jij geschikt voor een studie aan de universiteit?\n";

    // Ask for age.
    std::cout << prompt_list->year << "\n";
    std::cin >> input_int;
    if (!input_int || !(input_int > DATE_NORM_YEAR)) {
        goto end_err_invalid;
    }
    birth_year = input_int;
    time_holder->tm_year = birth_year - 1900 + 10;
    time_converted = mktime(time_holder);
    if (difftime(time_converted, time_now) > 0) {
        goto end_err_young;
    }

    std::cout << prompt_list->month << "\n";
    std::cin >> input_int;
    if (!input_int || (1 < input_int && input_int > 12)) {
        goto end_err_invalid;
    }
    birth_month = input_int;
    time_holder->tm_mon = birth_month - 1;
    time_converted = mktime(time_holder);
    if (difftime(time_converted, time_now) > 0) {
        goto end_err_young;
    }

    std::cout << prompt_list->day << "\n";
    std::cin >> input_int;
    if (!input_int || !(1 <= input_int && input_int <= 31)) {
        goto end_err_invalid;
    }
    birth_day = input_int;
    time_holder->tm_mday = birth_day;
    time_converted = mktime(time_holder);
    if (difftime(time_converted, time_now) > 0) {
        goto end_err_young;
    }

    // Should we ask informal questions? Rough calculation of age.
    time_holder->tm_year = time_holder->tm_year - 10;
    time_converted = mktime(time_holder);
    if (int(difftime(time_now, time_converted) / 31556925.9936) < LIMIT_FORMAL_AGE) {
        prompt_list = &prompt_list_informal;
    }

    // Calculate day of birthday.
    num_leap_days = int((birth_year - DATE_NORM_YEAR) / 4);
    if (((float)birth_year / 4 - int(birth_year / 4)) == 0.0 &&
        (birth_month <= 2))  // Correction if birthyear is leap year.
    {
        num_leap_days--;
    }

    num_days_total = (birth_year - DATE_NORM_YEAR) * DAYS_IN_YEAR + day_month[birth_month - 1] +
                     birth_day - 1 + num_leap_days;
    exp_day_str = day_list[num_days_total % 7];
    std::cout << birth_year - DATE_NORM_YEAR << "\n";

    // Ask for day of the birthday and test correctness.
    std::cout << prompt_list->exp_day_str << "\n";
    std::cin >> input_char;
    if (!input_char) {
        goto end_err_invalid;
    }
    switch (input_char | 0x20) {  // ugly switch case
        case 'z':
            std::cout << "> Welke dag:  za(a) of zo(o))?\n";
            std::cin >> input_char;
            input_char |= 0x20;  // lowercase
            if (!input_char || (input_char != 'a' && input_char != 'o')) {
                goto end_err_invalid;
            }
            if (input_char == 'a' && exp_day_str != DATE_DAY_SATERDAY) {
                goto end_err_day_str;
            } else if (input_char == 'o' && exp_day_str != DATE_DAY_SUNDAY) {
                goto end_err_day_str;
            }
            break;
        case 'm':
            if (exp_day_str != DATE_DAY_MONDAY) {
                goto end_err_day_str;
            }
            break;
        case 'd':
            std::cout << "> Welke dag: di(i) of do(o))?\n";
            std::cin >> input_char;
            input_char |= 0x20;  // lowercase
            if (!input_char || (input_char != 'i' && input_char != 'o')) {
                goto end_err_invalid;
            }
            if (input_char == 'i' && exp_day_str != DATE_DAY_TUESDAY) {
                goto end_err_day_str;
            } else if (input_char == 'o' && exp_day_str != DATE_DAY_THURSDAY) {
                goto end_err_day_str;
            }
            break;
        case 'w':
            if (exp_day_str != DATE_DAY_WEDNESDAY) {
                goto end_err_day_str;
            }
            break;
        case 'v':
            if (exp_day_str != DATE_DAY_FRIDAY) {
                goto end_err_day_str;
            }
            break;
        default:
            goto end_err_invalid;
    }

    // Initialze math question.
    math_sign = (rand() % 2) ? '+' : '-';
    math_1_teller = rand() % 20 + 1;
    math_1_noemer = rand() % 20 + 1;
    math_2_teller = rand() % 20 + 1;
    math_2_noemer = rand() % 20 + 1;
    if (math_1_noemer == math_2_noemer) {
        math_answer_teller = math_1_teller + (math_2_teller * ((math_sign == '-') ? -1 : 1));
        math_answer_noemer = math_1_noemer;
    } else {
        math_answer_teller = math_1_teller * math_2_noemer +
                             (math_2_teller * math_1_noemer * ((math_sign == '-') ? -1 : 1));
        math_answer_noemer = math_1_noemer * math_2_noemer;
    }
    math_answer_float = (float)math_answer_teller / (float)math_answer_noemer;

    // Ask math question.
    std::cout << prompt_list->math;
    std::cout << math_1_teller << "/" << math_1_noemer << " " << math_sign << " " << math_2_teller
              << "/" << math_2_noemer << " = ?\n";
    std::cout << prompt_list->math_teller << "\n";
    std::cin >> input_int;
    if (input_int != MATH_ANSWER_CHEAT && input_int != math_answer_teller) {
        goto end_math_fault;
    }

    std::cout << prompt_list->math_noemer << "\n";
    std::cin >> input_int;
    if (input_int != MATH_ANSWER_CHEAT && input_int != math_answer_noemer) {
        goto end_math_fault;
    }

    std::cout << prompt_list->math_float << "\n";
    std::cin >> input_float;
    if (input_float != MATH_ANSWER_CHEAT &&
        ((math_answer_float - MATH_ANSWER_FLOAT_LIMIT) > input_float ||
         input_float > (math_answer_float + MATH_ANSWER_FLOAT_LIMIT))) {
        goto end_math_fault;
    }

    std::cout << prompt_list->ok_exact << "\n";
    goto end;

end_math_fault:
    std::cout << prompt_list->math_fault << "\n";

    // Ask beta question.
    std::cout << question_item->question << "\n";
    std::cout << "\t" << question_item->choice_1 << "\n";
    std::cout << "\t" << question_item->choice_2 << "\n";
    std::cout << "\t" << question_item->choice_3 << "\n";
    std::cout << "\t" << question_item->choice_4 << "\n";
    std::cout << prompt_list->question << "\n";

    std::cin >> input_char;
    if (!input_char ||
        (input_char != 'a' && input_char != 'b' && input_char != 'c' && input_char != 'd')) {
        goto end_err_invalid;
    }
    if ((input_char | 0x20) != question_item->answer) {
        std::cout << prompt_list->question_fault;
        std::cout << " Het juiste antwoord was " << question_item->answer << ".\n";
    } else {
        std::cout << prompt_list->ok_beta << "\n";
    }

    goto end;

end_err_young:
    std::cout << "! Je bent te jong om deze vragenlijst in te vullen.\n";
    return 1;

end_err_day_str:
    std::cout << "! De opgegeven dag is niet valide, gegevens verwijderd!\n";
    return 1;

end_err_invalid:
    std::cout << "! De gegeven informatie is niet valide, probeer het opnieuw.\n";
    return 1;

end:
    std::cout << prompt_list->done << "\n";
    return 0;
}