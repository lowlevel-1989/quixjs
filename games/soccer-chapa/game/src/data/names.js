// games/soccer-chapa/game/src/data/names.js: los nombres de las chapas,
// tantas mujeres como hombres. Un apodo (a veces) o un nombre, y uno o dos
// apellidos: 143 nombres de hombre, 142 de mujer, 181 apellidos y 56 apodos
// (los de hombre, los de mujer y los de cualquiera) dan mas de un millon de
// combinaciones, para que casi nunca se repitan. Los mismos en espanol y en
// ingles.
// people: nombres completos ya hechos; pueden salir en cualquier carta, pero
// poco (SPECIAL: la probabilidad en cada chapa; con 4 por sobre, uno cada
// 12 sobres mas o menos), y pueden repetirse (las stats de cada una son otras).
SC.data.names = {
  SPECIAL: 0.02,
  people: ['Vinicio Valbuena', 'VickyBloom', 'Luis Valbuena', 'Manuel Valbuena', 'Aaron Hava',
            'Andres Quiroga', 'Jose Leiva'],
  men: [
    'Aaron', 'Abel', 'Adan', 'Adrian', 'Agustin', 'Aitor', 'Alan', 'Alberto', 'Aldo',
    'Alejandro', 'Alex', 'Alexis', 'Alfonso', 'Alfredo', 'Alonso', 'Alvaro', 'Andres',
    'Angel', 'Anibal', 'Antonio', 'Ariel', 'Armando', 'Arturo', 'Axel', 'Benjamin',
    'Bernardo', 'Boris', 'Bruno', 'Carlos', 'Cesar', 'Claudio', 'Cristian', 'Daniel',
    'Dante', 'Dario', 'David', 'Diego', 'Dylan', 'Edgar', 'Eduardo', 'Elias', 'Emiliano',
    'Emilio', 'Enrique', 'Enzo', 'Eric', 'Ernesto', 'Esteban', 'Ezequiel', 'Fabian',
    'Facundo', 'Federico', 'Felipe', 'Fernando', 'Fidel', 'Francisco', 'Franco',
    'Gabriel', 'Gael', 'Gaspar', 'Gerardo', 'German', 'Gonzalo', 'Gregorio', 'Guido',
    'Guillermo', 'Gustavo', 'Hector', 'Hernan', 'Horacio', 'Hugo', 'Ian', 'Ignacio',
    'Isaac', 'Ismael', 'Ivan', 'Jacobo', 'Jaime', 'Javier', 'Jesus', 'Joaquin', 'Jordi',
    'Jorge', 'Jose', 'Juan', 'Julian', 'Julio', 'Leandro', 'Leo', 'Leonardo', 'Lorenzo',
    'Lucas', 'Luciano', 'Luis', 'Manuel', 'Marcelo', 'Marco', 'Marcos', 'Mariano',
    'Mario', 'Martin', 'Mateo', 'Matias', 'Mauricio', 'Maximo', 'Miguel', 'Nahuel',
    'Nelson', 'Nicolas', 'Octavio', 'Oliver', 'Omar', 'Oscar', 'Pablo', 'Pascual',
    'Patricio', 'Pedro', 'Rafael', 'Ramiro', 'Ramon', 'Raul', 'Ricardo', 'Roberto',
    'Rodrigo', 'Rogelio', 'Roman', 'Ruben', 'Salvador', 'Samuel', 'Santiago',
    'Sebastian', 'Sergio', 'Simon', 'Tadeo', 'Teo', 'Tobias', 'Tomas', 'Ulises',
    'Valentin', 'Vicente', 'Victor', 'Walter', 'Yago'
  ],
  women: [
    'Abril', 'Adela', 'Agustina', 'Aitana', 'Alba', 'Alejandra', 'Alicia', 'Alma',
    'Amaia', 'Amalia', 'Ana', 'Andrea', 'Angela', 'Antonia', 'Ariadna', 'Aurora',
    'Barbara', 'Beatriz', 'Belen', 'Bianca', 'Blanca', 'Brenda', 'Camila', 'Candela',
    'Carla', 'Carmen', 'Carola', 'Carolina', 'Catalina', 'Cecilia', 'Celeste', 'Chiara',
    'Clara', 'Cristina', 'Dafne', 'Daniela', 'Debora', 'Delfina', 'Diana', 'Elena',
    'Elisa', 'Elsa', 'Emilia', 'Erika', 'Estela', 'Eva', 'Fatima', 'Fernanda',
    'Florencia', 'Gabriela', 'Gina', 'Gloria', 'Guadalupe', 'Helena', 'Ines', 'Irene',
    'Isabel', 'Ivana', 'Jazmin', 'Jimena', 'Josefina', 'Juana', 'Julia', 'Karina',
    'Lara', 'Laura', 'Lia', 'Lola', 'Lorena', 'Lourdes', 'Lucia', 'Lucila', 'Luna',
    'Macarena', 'Magdalena', 'Malena', 'Manuela', 'Mara', 'Margarita', 'Maria',
    'Mariana', 'Marina', 'Marta', 'Martina', 'Matilde', 'Melina', 'Mercedes', 'Micaela',
    'Milena', 'Miriam', 'Mirta', 'Monica', 'Nadia', 'Natalia', 'Nerea', 'Noa', 'Noelia',
    'Nora', 'Norma', 'Olga', 'Olivia', 'Paola', 'Patricia', 'Paula', 'Pia', 'Pilar',
    'Raquel', 'Rebeca', 'Regina', 'Renata', 'Rocio', 'Romina', 'Rosa', 'Rufina',
    'Sabrina', 'Sara', 'Selena', 'Serena', 'Silvana', 'Silvia', 'Sofia', 'Sol', 'Susana',
    'Tamara', 'Tania', 'Teresa', 'Uma', 'Valentina', 'Valeria', 'Vanesa', 'Vera',
    'Victoria', 'Violeta', 'Virginia', 'Wanda', 'Ximena', 'Yanina', 'Yasmin', 'Yolanda',
    'Zaira', 'Zoe', 'Zulema'
  ],
  last: [
    'Abreu', 'Acosta', 'Acuna', 'Aguilar', 'Aguirre', 'Alarcon', 'Alba', 'Albornoz',
    'Alcaraz', 'Almada', 'Alonso', 'Alvarado', 'Alvarez', 'Amaya', 'Andrade', 'Aranda',
    'Araujo', 'Arce', 'Arias', 'Arroyo', 'Avila', 'Ayala', 'Baez', 'Ballesteros',
    'Barrera', 'Barrios', 'Bassi', 'Bautista', 'Bello', 'Benitez', 'Bermudez', 'Blanco',
    'Bravo', 'Bustos', 'Caballero', 'Cabrera', 'Calderon', 'Campos', 'Cano', 'Cardenas',
    'Carmona', 'Carrasco', 'Carrillo', 'Castillo', 'Castro', 'Cervantes', 'Chavez', 'Cisneros',
    'Colombo', 'Contreras', 'Cordoba', 'Correa', 'Cortes', 'Costa', 'Cruz', 'Cuevas',
    'Delgado', 'Diaz', 'Dominguez', 'Duarte', 'Duran', 'Escobar', 'Espinoza', 'Esposito',
    'Estrada', 'Farias', 'Fernandez', 'Ferrari', 'Ferreyra', 'Figueroa', 'Flores', 'Franco',
    'Fuentes', 'Gallardo', 'Gallego', 'Galvez', 'Garcia', 'Garrido', 'Gil', 'Gimenez',
    'Godoy', 'Gomez', 'Gonzalez', 'Guerra', 'Guerrero', 'Gutierrez', 'Guzman', 'Herrera',
    'Hidalgo', 'Ibarra', 'Iglesias', 'Juarez', 'Ledesma', 'Leon', 'Lopez', 'Lozano',
    'Lucero', 'Luna', 'Maldonado', 'Marin', 'Marquez', 'Martin', 'Martinez', 'Medina',
    'Mejia', 'Mendez', 'Mendoza', 'Miranda', 'Molina', 'Montero', 'Montes', 'Mora',
    'Morales', 'Moreno', 'Munoz', 'Navarro', 'Nieto', 'Nunez', 'Ocampo', 'Ojeda',
    'Olivera', 'Ortega', 'Ortiz', 'Pacheco', 'Padilla', 'Paez', 'Palacios', 'Paredes',
    'Pena', 'Peralta', 'Perez', 'Pineda', 'Ponce', 'Prieto', 'Quintero', 'Quiroga',
    'Ramirez', 'Ramos', 'Reyes', 'Rinaldi', 'Rios', 'Rivas', 'Rivera', 'Robles',
    'Rodriguez', 'Rojas', 'Romero', 'Rosales', 'Rossi', 'Rubio', 'Ruiz', 'Salas',
    'Salazar', 'Salinas', 'Sanchez', 'Sandoval', 'Santana', 'Santos', 'Sepulveda', 'Serrano',
    'Silva', 'Sosa', 'Soto', 'Suarez', 'Tapia', 'Toledo', 'Torres', 'Valdez',
    'Valencia', 'Varela', 'Vargas', 'Vazquez', 'Vega', 'Velazquez', 'Vera', 'Vidal',
    'Villalba', 'Villanueva', 'Zamora', 'Zapata', 'Zarate'
  ],
  nickMen: [
    'Cholo', 'El Capi', 'El Chino', 'El Flaco', 'El Gordo', 'El Loco', 'El Mago',
    'El Mono', 'El Muro', 'El Negro', 'El Niño', 'El Pibe', 'El Profe', 'El Pulpo',
    'El Rayo', 'El Rubio', 'El Ruso', 'El Tano', 'El Tanque', 'El Toro', 'El Turco',
    'Kiko', 'Lalo', 'Nene', 'Pepe', 'Pipo', 'Rulo', 'Tito', 'Toto'
  ],
  nickWomen: [
    'La Bala', 'La Bruja', 'La Capi', 'La Flaca', 'La Flecha', 'La Gata', 'La Leona',
    'La Muralla', 'La Profe', 'La Pulga', 'La Reina', 'La Roca', 'La Rubia', 'La Tana',
    'La Zurda', 'Lali', 'Luli', 'Maru', 'Nena', 'Pili', 'Tati'
  ],
  nickAny: [
    'Bocha', 'Chiqui', 'Chispa', 'Cuqui', 'Peque', 'Tucu'
  ]
};

// un nombre: 'Leo Benitez', 'La Zurda Rojas' o 'Lucia Paredes Ortiz'; mitad
// mujeres, mitad hombres (el apodo, del mismo)
SC.data.names.make = function () {
  var n = SC.data.names, woman = Math.random() < 0.5;
  var a = Math.random() < 0.25 ? SC.data.random.pick((woman ? n.nickWomen : n.nickMen).concat(n.nickAny)) : SC.data.random.pick(woman ? n.women : n.men);
  var b = SC.data.random.pick(n.last);
  if (Math.random() < 0.3) b += ' ' + SC.data.random.pick(n.last);
  return a + ' ' + b;
};

// a veces, uno de los nombres completos (people); si no, null
SC.data.names.special = function () {
  return Math.random() < SC.data.names.SPECIAL ? SC.data.random.pick(SC.data.names.people) : null;
};

// solo un nombre de pila (o un apodo) y solo un apellido (a veces dos), como
// los de SC.data.names.make (firstName, lastName: last es la lista): las
// cartas de cambio de nombre
SC.data.names.firstName = function () {
  var n = SC.data.names, woman = Math.random() < 0.5;
  return Math.random() < 0.25 ? SC.data.random.pick((woman ? n.nickWomen : n.nickMen).concat(n.nickAny)) : SC.data.random.pick(woman ? n.women : n.men);
};
SC.data.names.lastName = function () {
  var n = SC.data.names, b = SC.data.random.pick(n.last);
  return Math.random() < 0.3 ? b + ' ' + SC.data.random.pick(n.last) : b;
};

// separa el nombre de una chapa en { first, last }: el nombre de pila (o el
// apodo mas largo de las listas con que empieza) y lo demas
SC.data.names.split = function (name) {
  var n = SC.data.names, best = name.split(' ')[0];
  n.women.concat(n.men, n.nickWomen, n.nickMen, n.nickAny).forEach(function (f) {
    if (f.length > best.length && name.indexOf(f + ' ') === 0) best = f;
  });
  return { first: best, last: name.slice(best.length + 1) };
};

// cambia los nombres repetidos de una lista de chapas
SC.data.names.unique = function (list) {
  var seen = {};
  list.forEach(function (c) {
    while (seen[c.name]) c.name = SC.data.names.make();
    seen[c.name] = true;
  });
};
