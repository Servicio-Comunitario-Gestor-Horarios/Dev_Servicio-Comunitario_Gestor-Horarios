# Docente
    - No puede estar en dos lugares al mismo tiempo
    - Tiene una serie de horas disponibles
    - Tienen que cumplir unas horas semanas
    - Si sus horas a cumplir son 40 o mas, 36 son en aula y 4 son de planificacion (para >40, se rellena en planificacion)
    - Opcional: Aparte de las materias que dicta un docente, debemos poder listar materias que en caso de emergencia el pueda cubrir (no es requerido que lo haga, es solo una lista que se usara en caso de no haber mas docentes para el puesto)

# Aula
    - No puede estar ocupado por dos cursos al mismo tiempo
    - Su configuracion en el solver debe ser flexible: A veces los cursos tienen un salon fijo, y otras veces son los docentes que tienen el salon fijo
    - Opcional (o restriccion blanda) debe cumplir con la capacidad de albergar a todos los estudiantes de un curso
    - En un aula no se pueden ver 2 materias al mismo tiempo

# Materia
    - Varios profesores pueden impartir una misma materia
    - La cantidad de horas semanales y los cursos que la ven estan ancladas al plan de estudio

# Plan de estudio
    - Agrupa un conjunto de materias, que cursos las deben ver, y cuantas horas a la semana
    - Un mismo curso (año y seccion) no puede ver 2 planes de estudio en simultaneo

# Curso (año y seccion)
    - A veces su salon es fijo, a veces tienen que cambiar
    - Se separan por turno mañana y tarde
    - La separacion por turnos puede ser muy arbitraria, a veces es por años, a veces por planes de estudio

# Franja Horaia y Turnos
    - Un turno debe poder ser dividido de manera exacta entre la duracion de la franja horaria de inicio a fin, restando el tiempo de receso
    - El receso se situa entre dos franjas horarias

# Horario
    - Debemos poder generar un recopilado de horarios donde se listen todos los cursos sin solapamientos
    - Cada curso tiene su propio horario
    - Con el horario entero ya generado debemos poder sacar el horario de cada profesor
    - Debemos poder hacer una generacion de horarios parciales (solo algunos cursos)
    - Para la generacion parcial debemos poder cargar un conjunto de horarios de cursos ya hechos o hacerlo totalmente desde 0