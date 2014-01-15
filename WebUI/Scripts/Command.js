function Command() {
    this.idle = false;
    this.ignition = false;
    this.neutral = false;
    this.repeat = 1;
    this.direction = 0;
    this.dirspeed = 0;
    this.rotation = 0;
    this.rotspeed = 0;
    this.turrelRotation = 0;
    this.canonElevation = false;
    this.fire = false;
    this.gun = false;
    this.engineStart = false;
    this.engineStop = false;
    this.recoil = false;
}

Command.prototype.toJSON = function () {
    return '{\n'+                                       
           '\t"Command":\n'+                        
           '\t{\n'+                                    
           '\t\t"idle": ' + this.idle + ',\n'+                
           '\t\t"ignition": ' + this.ignition + ',\n' +
            '\t\t"neutral" : ' + this.neutral + ',\n' +
            '\t\t"repeat" : ' + this.repeat + ',\n' +
            '\t\t"direction" : ' + this.direction + ',\n' +
            '\t\t"dirspeed" : ' + this.dirspeed + ',\n' +
            '\t\t"rotation": ' + this.rotation + ',\n' +
            '\t\t"rotspeed" : ' + this.rotspeed + ',\n' +
            '\t\t"turrelRotation" : ' + this.turrelRotation + ',\n' +
            '\t\t"canonElevation" : ' + this.canonElevation + ',\n' +
            '\t\t"fire" : ' + this.fire + ',\n' +
            '\t\t"gun" : ' + this.gun + ',\n' +
            '\t\t"engineStart" : ' + this.engineStart + ',\n' +
            '\t\t"engineStop" : ' + this.engineStop + ',\n' +
            '\t\t"recoil" : ' + this.recoil + '\n' +
            '\t}\n'+                                  
            '}';
}

Command.prototype.equals = function (other) {
    if (other != null && other.prototype == this) {
        if (this.idle == other.idle &&
            this.ignition == other.ignition &&
            this.neutral == other.neutral &&
            this.repeat == other.repeat &&
            this.direction == other.direction &&
            this.dirspeed == other.dirspeed &&
            this.rotation == other.rotation &&
            this.rotspeed == other.rotspeed &&
            this.turrelRotation == other.turrelRotation &&
            this.canonElevation == other.canonElevation &&
            this.fire == other.fire &&
            this.gun == other.gun &&
            this.engineStart == other.engineStart &&
            this.engineStop == other.engineStop &&
            this.recoil == other.recoil)
            return true;
    }
    return false;
}