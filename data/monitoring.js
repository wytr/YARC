const data = {
    labels: [],
    datasets: [
      {
        label: 'Target',
        data: [],
        borderColor: 'rgb(241, 90, 41)',
        backgroundColor: 'rgb(241, 90, 41)',
      },
      {
        label: 'Actual',
        data: [],
        borderColor: 'rgb(0, 0, 0)',
        backgroundColor:'rgb(0, 0, 0)',
      }
    ]
};

const config = {
    type: 'line',
    data: data,
    options: {
      animation: true,
      responsive: true,
      plugins: {
        legend: {
          position: 'top',
        }        
      },
      datasets: {
        line: {
            pointRadius: 0
        }
      },
      scales: {
        x: {
            ticks: {
                display: false 
            },
            grid: {
                display: false
            }
        },
        y: {
            type: 'linear',
            min: 0,
            max: 350
        }
      }
    },
  };

var myChart;

function initChart() {
    var ctx = document.getElementById('myChart').getContext('2d');
    myChart = new Chart(ctx,config);
}

function addData(label, target, actual) {
    myChart.data.labels.push(label);
    myChart.data.datasets[0].data.push(target);
    myChart.data.datasets[1].data.push(actual);
    myChart.update();
}

function startMonitoring() {
    sendMessage("startMonitoring");
}

function stopMonitoring() {
    sendMessage("stopMonitoring");
}

function clearChart() {
    myChart.data.datasets[0].data = [];
    myChart.data.datasets[1].data = [];
    myChart.data.labels = [];
    myChart.update();
}

function download() {
    var fileName = "ReflowData-" + Date() + ".csv";
    var csvContent = "";    
    for(var i = 0; i < myChart.data.labels.length; i++) {
        csvContent += myChart.data.labels[i] + ";" + myChart.data.datasets[0].data[i] + ";" + myChart.data.datasets[1].data[i] + '\n';
    }

    var a = document.createElement('a');
    mimeType = 'text/csv';
    if (navigator.msSaveBlob) { // IE10
        return navigator.msSaveBlob(new Blob([csvContent], { type: mimeType }), fileName);
    } else if ('download' in a) { //html5 A[download]
        a.href = 'data:' + mimeType + ',' + encodeURIComponent(csvContent);
        a.setAttribute('download', fileName);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    }
} 

window.addEventListener('load', _ => {  
    if(!myChart){
        initChart();
    }    
});
