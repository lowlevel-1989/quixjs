function (original) {
  return function () {
    Game.never = true;
    return original.apply(this, arguments);
  };
}
