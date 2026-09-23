#include "subareablocks.h"

subareaBlocks::subareaBlocks()
{

}

void subareaBlocks::cal_rc(Datapoint &datapoint)
{
    QVector<double> xx,yy;
    for (const auto elem:datapoint)
    {
        xx.push_back(elem.second.X);
        yy.push_back(elem.second.Y);
    }
    if (xx.isEmpty())
    {
        qWarning() << "subareaBlocks::cal_rc: datapoint is empty, aborting row/col calculation";
        col_in = 0;
        row_in = 0;
        return;
    }
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    col_in = round((maxX-minX)/x_step +1);
    row_in = round((maxY-minY)/y_step +1);
}

void subareaBlocks::dp2anop(Datapoint &datapoint)
{
    QVector<AnoPoint> temp;
    for (const auto elem:datapoint)
    {
        AnoPoint t;
        t.x = elem.second.X;
        t.y = elem.second.Y;
        t.z = elem.second.tMagnetic;
        temp.push_back(t);
    }
    dataInput.resize(row_in);
    for (int i = 0; i < row_in; ++i)
    {
        dataInput[i].resize(col_in);
        for (int j = 0; j < col_in; ++j)
        {
            int index = i * col_in + j;
            dataInput[i][j] = (index < temp.size()) ? temp[index] : AnoPoint{0.0, 0.0, 0.0};
        }
    }
}





double subareaBlocks::calculateStandardDeviation(const QVector<double>& data) {
    if (data.empty())
    {
        return 0;
    }
    double sum = 0.0;
    for (double value : data)
    {
        sum += value;
    }
    double mean = sum / data.size();
    double sumOfSquares = 0.0;
    for (double value : data)
    {
        sumOfSquares += pow(value - mean, 2);
    }
    double variance = sumOfSquares / data.size(); // 方差
    return sqrt(variance); // 标准差是方差的平方根
}

void subareaBlocks::createBounds()
{
    // 计算每一块的大小
    int groupSize_row = ceil((row_in + (row_blockCount -1) * row_overlap )/row_blockCount);
    int groupSize_col = ceil((col_in + (col_blockCount -1) * col_overlap )/col_blockCount);

    // 计算每一块数据的起始
    QVector<std::pair<int,int>> i_index;
    QVector<std::pair<int,int>> j_index;
    for (int i = 0;i<row_blockCount-1;i++)
    {
        std::pair<int,int> temp;
        temp.first  = qBound(0, i*(groupSize_row - row_overlap), row_in - 1);
        temp.second = qBound(0, i*(groupSize_row - row_overlap) + groupSize_row - 1, row_in - 1);
        i_index.push_back(temp);
    }
    std::pair<int,int> temp0;
    temp0.first  = qBound(0, (row_blockCount-1)*(groupSize_row - row_overlap), row_in - 1);
    temp0.second = row_in-1;
    i_index.push_back(temp0);
    for (int j = 0;j<col_blockCount-1;j++)
    {
        std::pair<int,int> temp;
        temp.first  = qBound(0, j*(groupSize_col - col_overlap), col_in - 1);
        temp.second = qBound(0, j*(groupSize_col - col_overlap) + groupSize_col - 1, col_in - 1);
        j_index.push_back(temp);
    }
    std::pair<int,int> temp1;
    temp1.first  = qBound(0, (col_blockCount-1)*(groupSize_col - col_overlap), col_in - 1);
    temp1.second = col_in-1;
    j_index.push_back(temp1);
    //
    ijBounds.resize(row_blockCount);
    for (int i = 0; i < row_blockCount; ++i)
    {
        ijBounds[i].resize(col_blockCount);
    }
    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            ijBound temp;
            temp.i_min = i_index[i].first;
            temp.i_max = i_index[i].second;
            temp.j_min = j_index[j].first;
            temp.j_max = j_index[j].second;
            ijBounds[i][j] = temp;
        }
    }

}

QVector<double>  subareaBlocks::extractSubRange(ijBound ij)
{
    QVector<double> result;
    // 遍历指定的行范围
    for (int i = ij.i_min; i <= ij.i_max; ++i)
    {
        // 遍历指定的列范围
        for (int j = ij.j_min; j <= ij.j_max; ++j)
        {
            result.push_back(dataInput[i][j].z);
        }
    }
    return result;
}


void subareaBlocks::subStd()
{
    substd.resize(row_blockCount);
    for (int i = 0; i < row_blockCount; ++i)
    {
        substd[i].resize(col_blockCount);
    }
    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            QVector<double> temp = extractSubRange(ijBounds[i][j]);
            substd[i][j] = calculateStandardDeviation(temp);
        }
    }
}

Datapoint subareaBlocks::anop2dp(ijBound ij)
{
    Datapoint datapoints;
    int flag = 1;
    // 遍历指定的行范围
    for (int i = ij.i_min; i <= ij.i_max; ++i)
    {
        // 遍历指定的列范围
        for (int j = ij.j_min; j <= ij.j_max; ++j)
        {
            SinglePoint point;
            point.X = dataInput[i][j].x;
            point.Y = dataInput[i][j].y;
            point.tMagnetic = dataInput[i][j].z;
            datapoints.insert(std::make_pair(flag, point));
            flag++;
        }
    }
    return datapoints;
}

void subareaBlocks::create_dp_result()
{
    // 初始化
    datapoint_result.resize(row_blockCount);
    for (int i = 0; i < row_blockCount; ++i)
    {
        datapoint_result[i].resize(col_blockCount);
    }
    for (int i = 0; i < row_blockCount; i++)
    {
        for (int j = 0; j < col_blockCount; j++)
        {
            int flag = 1;

            // 获取当前分块的物理坐标范围
            double x_start = dataInput[ijBounds[i][j].i_min][ijBounds[i][j].j_min].x;
            double y_start = dataInput[ijBounds[i][j].i_min][ijBounds[i][j].j_min].y;
            double x_end = dataInput[ijBounds[i][j].i_max][ijBounds[i][j].j_max].x;
            double y_end = dataInput[ijBounds[i][j].i_max][ijBounds[i][j].j_max].y;

            // 确保起点对齐到 0.5 的整数倍（避免 -0.25 这样的偏移）
            x_start = std::round(x_start / 0.5) * 0.5;
            y_start = std::round(y_start / 0.5) * 0.5;

            // 计算该分块的行列数
            int cols = static_cast<int>(std::round((x_end - x_start) / 0.5)) + 1;
            int rows = static_cast<int>(std::round((y_end - y_start) / 0.5)) + 1;

            // 重新计算终点，确保不超出输入范围
            x_end = x_start + (cols - 1) * 0.5;
            y_end = y_start + (rows - 1) * 0.5;

            // 生成网格点
            for (int ii = 0; ii < rows; ii++)
            {
                for (int jj = 0; jj < cols; jj++)
                {
                    SinglePoint point;
                    point.X = x_start + 0.5 * jj;  // X 方向（列）
                    point.Y = y_start + 0.5 * ii;  // Y 方向（行）
                    point.tMagnetic = 0.0;
                    datapoint_result[i][j].insert(std::make_pair(flag, point));
                    flag++;
                }
            }
        }
    }
}

void subareaBlocks::subModel(Datainfo datainfo)
{

    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            QVector<AnoPoint> subresult;
            // Polyhedral
            Geomagnetic::Polyhedral poly;
            Geomagnetic::Datapoint tmp_dp = anop2dp(ijBounds[i][j]);
            ReadData readdata;
            readdata.DataSet(tmp_dp, datainfo);
            datainfo.PolyQ = 0;
            datainfo.sigma2 = 10.0;
            poly.init(datainfo, tmp_dp);
            poly.ComputeQ(datainfo, tmp_dp);
            poly.ComputeX(datainfo, tmp_dp);
            poly.Result(datainfo, datapoint_result[i][j],tmp_dp);
        }
    }
}



// Joins the block results into one grid (step 0.5, window 3, equal weights).  Blocks overlap, so
// neither a level adjustment nor gross-error rejection is wanted here.
void subareaBlocks::submerge(Datapoint &all)
{
    if (all.empty())
    {
        qWarning() << "subareaBlocks::submerge: 'all' is empty, aborting submerge";
        return;
    }
    double min_x = std::numeric_limits<double>::max(), max_x = std::numeric_limits<double>::lowest();
    double min_y = min_x, max_y = max_x;
    for (const auto &elem : all)
    {
        min_x = std::min(min_x, elem.second.X);
        max_x = std::max(max_x, elem.second.X);
        min_y = std::min(min_y, elem.second.Y);
        max_y = std::max(max_y, elem.second.Y);
    }
    const double step = 0.5;

    std::vector<Proc::FusionSource> sources(1);
    sources[0].name = QStringLiteral("blocks");
    for (int i = 0; i < row_blockCount; i++)
        for (int j = 0; j < col_blockCount; j++)
            for (const auto &elem : datapoint_result[i][j])
                sources[0].samples.push_back({elem.second.X, elem.second.Y, elem.second.tMagnetic});

    Proc::GridSpec grid;
    grid.xMin = std::floor(min_x / step) * step;
    grid.xMax = std::ceil(max_x / step) * step;
    grid.yMin = std::floor(min_y / step) * step;
    grid.yMax = std::ceil(max_y / step) * step;
    grid.dx = grid.dy = step;
    Proc::FusionOptions options;
    options.windowX = options.windowY = 3.0;
    options.removeBias = false;
    options.rejectOutliers = false;
    const Proc::FusionResult fused = Proc::fuse(std::move(sources), grid, options);
    if (!fused.ok())
    {
        qWarning() << "subareaBlocks::submerge:" << fused.error;
        return;
    }
    for (const Proc::Sample &s : Proc::gridToSamples(fused.grid))
        result.push_back({s.y, s.x, s.v});
}

double subareaBlocks::distanceBetween(SinglePoint p1,AnoPoint p2)
{
    return std::sqrt(std::pow(p1.X - p2.x, 2) + std::pow(p1.Y - p2.y, 2));
}

SinglePoint subareaBlocks::findNearestPoint(Datapoint input, AnoPoint p)
{
    if (input.empty())
    {
        return SinglePoint(); // 空结果集时返回默认点，避免 .at() 抛出异常导致程序崩溃
    }
    SinglePoint nearestPoint = input.begin()->second; // 假设第一个点是最接近的，以便开始
    double minDistance = distanceBetween(nearestPoint,p);
    for (const auto point : input)
    {
        double distance = distanceBetween(point.second,p);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestPoint = point.second;
        }
    }
    return nearestPoint;
}

void subareaBlocks::createSubAll(Datapoint &all)
{
    QVector<double> xx,yy;
    for (const auto elem:all)
    {
        xx.push_back(elem.second.X);
        yy.push_back(elem.second.Y);
    }
    if (xx.isEmpty())
    {
        qWarning() << "subareaBlocks::createSubAll: 'all' is empty, aborting createSubAll";
        return;
    }
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    int all_col_in = round((maxX-minX)/0.5 +1);
    int all_row_in = round((maxY-minY)/0.5 +1);

    QVector<AnoPoint> temp;
    for (const auto elem:all)
    {
        AnoPoint t;
        t.x = elem.second.X;
        t.y = elem.second.Y;
        t.z = elem.second.tMagnetic;
        temp.push_back(t);
    }
    //
    dataInput_all.resize(all_row_in);
    for (int i = 0; i < all_row_in; ++i)
    {
        dataInput_all[i].resize(all_col_in);
        for (int j = 0; j < all_col_in; ++j)
        {
            int index = i * all_col_in + j;
            dataInput_all[i][j] = (index < temp.size()) ? temp[index] : AnoPoint{0.0, 0.0, 0.0};
        }
    }
    //
    subInput.resize(row_blockCount);
    for (int i = 0; i < row_blockCount; ++i)
    {
        subInput[i].resize(col_blockCount);
    }
    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            int flag = 1;
            double i_start = dataInput[ijBounds[i][j].i_min][ijBounds[i][j].j_min].x;
            double j_start = dataInput[ijBounds[i][j].i_min][ijBounds[i][j].j_min].y;
            double i_end = dataInput[ijBounds[i][j].i_max][ijBounds[i][j].j_max].x;
            double j_end = dataInput[ijBounds[i][j].i_max][ijBounds[i][j].j_max].y;
            int col_jj = round((j_end-j_start)/0.5 +1); // 该区行列数
            int row_ii = round((i_end-i_start)/0.5 +1);
            int ii_start = round((i_start-minX)/0.5);
            int jj_start = round((j_start-minY)/0.5);
            int ii_end = ii_start+row_ii;
            int jj_end = jj_start+col_jj;
            for (int ii = ii_start;ii<ii_end;ii++)
            {
                for (int jj = jj_start;jj<jj_end;jj++)
                {
                    subInput[i][j].push_back(dataInput_all[jj][ii]);
                }
            }
        }
    }
}

void subareaBlocks::subAccuracy(Datapoint &all)
{
    // 寻找最近点并计算rms
    subrms.resize(row_blockCount);
    for (int i = 0; i < row_blockCount; ++i)
    {
        subrms[i].resize(col_blockCount);
    }
    double SumSquares = 0.0;
    int CountSumSquares = 0;
    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            double subSumSquares = 0.0;
            int CountsubSumSquares = 0;
            for (auto elem:subInput[i][j])
            {
                SinglePoint p = findNearestPoint(datapoint_result[i][j],elem);
                subSumSquares = subSumSquares + (p.tMagnetic - elem.z) * (p.tMagnetic - elem.z);
                CountSumSquares++;
                SumSquares = SumSquares + (p.tMagnetic - elem.z) * (p.tMagnetic - elem.z);
                CountsubSumSquares++;
            }
            subrms[i][j] = (CountsubSumSquares > 0) ? qSqrt(subSumSquares/CountsubSumSquares) : 0.0;
        }
    }
    rms = (CountSumSquares > 0) ? qSqrt(SumSquares/CountSumSquares) : 0.0;
}

void subareaBlocks::out2file(QString filepath)
{
    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug()<<"failed: "<<file.errorString();
        outstr = "保存文件失败: " + file.errorString();   // surface to user via existing outstr mechanism used elsewhere in this class
        return;
    }
    QTextStream out(&file);
    for (auto elem:result)
    {
        if(!(isnan(elem.z)))
            out<<elem.y<<" "<<elem.x<<" "<<elem.z<<endl;
    }
    file.close();
}

int subareaBlocks::inputPara_subarea(QDialog &dialog,int &data_num, QStringList &data_name_list,int &data_index,QString &dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("分区-输入参数: ");

    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    spinbox1->setValue(0.5);
    form.addRow("X分辨率(KM): ", spinbox1);
    // #3
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    spinbox2->setValue(2.0);
    form.addRow("Y分辨率(KM): ", spinbox2);
    // #4
    QSpinBox *spinbox3 = new QSpinBox(&dialog);
    spinbox3->setValue(2);
    form.addRow("行重叠度（行）: ", spinbox3);
    // #5
    QSpinBox *spinbox4 = new QSpinBox(&dialog);
    spinbox4->setValue(2);
    form.addRow("列重叠度（列）: ", spinbox4);
    // #6
    QSpinBox *spinbox5 = new QSpinBox(&dialog);
    spinbox5->setValue(2);
    spinbox5->setMaximum(6);
    form.addRow("分成列数: ", spinbox5);
    // #7
    QSpinBox *spinbox6 = new QSpinBox(&dialog);
    spinbox6->setValue(2);
    spinbox6->setMaximum(6);
    form.addRow("分成行数: ", spinbox6);
    //    // #8
    //    QLineEdit *lineEdit = new QLineEdit(&dialog);
    //    form.addRow("另存为: ", lineEdit);
    // #9
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        if(comboBox->count()==0)
            return -1;
        data_index = comboBox->currentIndex();
        x_step = spinbox1->value();
        y_step = spinbox2->value();
        row_overlap = spinbox3->value();
        col_overlap = spinbox4->value();
        row_blockCount = spinbox5->value();
        col_blockCount = spinbox6->value();
        return 0;
    }
    else
        return -1;
}

void subareaBlocks::subarea(Datapoint &all,Datainfo datainfo,Datapoint &datapoint)
{
    cal_rc(datapoint);
    dp2anop(datapoint);
    createBounds();
}

void subareaBlocks::build(Datapoint &all,Datainfo datainfo,Datapoint &datapoint,QString filepath)
{
    subStd();
    create_dp_result();
    subModel(datainfo);
    submerge(all);
    createSubAll(all);
    subAccuracy(all);
    out2file(filepath);
    outstr = "各区RMS: \n";
    for (int i = 0;i<row_blockCount;i++)
    {
        for (int j = 0;j<col_blockCount;j++)
        {
            outstr = outstr+ "第" +QString::number(i+1) + "行 - 第"
                     + QString::number(j+1)+"列 : "+QString::number(subrms[i][j], 'f', 3) + "\n";
        }
    }
    outstr = outstr+"总RMS: "+QString::number(rms, 'f', 3);

}
