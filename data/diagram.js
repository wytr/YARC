function setCooldownHeight() {    
    var reflowTemperature = document.getElementById('reflowTemperature').value;
    document.getElementById('cooldown').setAttribute('style',  'border-bottom:' + reflowTemperature + 'em solid red');
}

function setSoakRampDimensions() {
    var soakRampDuration = document.getElementById('soakTemperature').value / document.getElementById('soakRampRate').value;
    var widthCss = 'border-left:' + soakRampDuration + 'em solid transparent;';
    var soakTemperature = document.getElementById('soakTemperature').value;
    var heightCss = 'border-bottom:' + soakTemperature + 'em solid red;';
    document.getElementById('soakRamp').setAttribute('style', widthCss + heightCss);
}

function setSoakDimensions() {
    var soakTemperature = document.getElementById('soakTemperature').value
    var soakDuration = document.getElementById('soakDuration').value
    document.getElementById('soak').setAttribute('style',  `height:${soakTemperature}em;width:${soakDuration}em`);
}

function setReflowRampDimensions() {     
    var soakTemperature = document.getElementById('soakTemperature').value;
    var reflowTemperature = document.getElementById('reflowTemperature').value;
    var reflowRampDuration = (reflowTemperature - soakTemperature) / document.getElementById('reflowRampRate').value;  
    var widthCss = 'border-left:' + reflowRampDuration + 'em solid transparent;';
    var heightCss = 'border-bottom:' + (reflowTemperature - soakTemperature) + 'em solid red;';
    document.getElementById('reflowRamp').setAttribute('style', widthCss + heightCss); 
}

function setReflowDimenions() {
    var reflowTemperature = document.getElementById('reflowTemperature').value
    var reflowDuration = document.getElementById('reflowDuration').value
    document.getElementById('reflow').setAttribute('style', `height:${reflowTemperature}em;width:${reflowDuration}em`);
}

function setRowHeights() {
    var soakTemperature = document.getElementById('soakTemperature').value;
    var reflowTemperature = document.getElementById('reflowTemperature').value;
    document.getElementsByClassName('grid-container')[0].setAttribute('style',  `grid-template-rows: ${reflowTemperature - soakTemperature}em ${soakTemperature}em 20em;`);
}

function setReflowRampSpacerWidth() {
    var soakRampDuration = document.getElementById('soakTemperature').value / document.getElementById('soakRampRate').value;
    var soakDuration = document.getElementById('soakDuration').value;
    document.getElementById('reflowRampSpacer').setAttribute('style',  'width:' + (+soakRampDuration + +soakDuration) + 'em');
}

document.addEventListener('DOMContentLoaded', function() {
	document.getElementById('soakRampRate').addEventListener('change', _ => {
        setSoakRampDimensions()
        setReflowRampSpacerWidth();
    });    
    document.getElementById('soakTemperature').addEventListener('change', _ => {
        setRowHeights();
        setSoakRampDimensions();
        setSoakDimensions()
        setCooldownHeight();
        setReflowRampSpacerWidth();
        setReflowRampDimensions();
    });    
    document.getElementById('soakDuration').addEventListener('change', _ => {
        setSoakDimensions()
        setReflowRampSpacerWidth();
    });    
    document.getElementById('reflowRampRate').addEventListener('change', _ => {
        setReflowRampDimensions();
    });    
    document.getElementById('reflowTemperature').addEventListener('change', _ => {
        setRowHeights();
        setReflowRampDimensions();
        setReflowDimenions();
        setCooldownHeight();
    });    
    document.getElementById('reflowDuration').addEventListener('change', _ => {
        setReflowDimenions();
    });    
});